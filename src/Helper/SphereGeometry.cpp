#include "SphereGeometry.h"
#include <cmath>

void SphereGeometry::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    setupSphere();

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void SphereGeometry::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);


    // Offset for position
    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    int coordLocation = program->attributeLocation("aCoord");
    program->enableAttributeArray(coordLocation);
    program->setAttributeBuffer(coordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Offset for normal
    offset += sizeof(QVector2D);

    int normalLocation = program->attributeLocation("aNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    program->release();
}

void
SphereGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection,
                                QOpenGLTexture *texture) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);
    program->setUniformValue("colorMap", 0);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    texture->bind();
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

void
SphereGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    glDrawElements(GL_TRIANGLE_STRIP, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

QVector<VertexData> SphereGeometry::getVerticesData() {
    return vertices;
}

QVector<GLuint> SphereGeometry::getIndices() {
    return indices;
}

void SphereGeometry::setupSphere() {
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            QVector3D pos;
            QVector2D tex;
            QVector3D normal;
            VertexData data;

            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            pos.setX(xPos);
            pos.setY(yPos);
            pos.setZ(zPos);

            tex.setX(xSegment);
            tex.setY(ySegment);

            normal.setX(xPos);
            normal.setY(yPos);
            normal.setZ(zPos);

            data.position = pos;
            data.texCoord = tex;
            data.normal = normal;

            vertices.push_back(data);
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
}
