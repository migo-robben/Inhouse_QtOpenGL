#include "FloorGeometry.h"


void FloorGeometry::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void
FloorGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection,
                            QOpenGLTexture *texture) {

    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);
    program->setUniformValue("map", 0);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    texture->bind();
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)nullptr);
}

void FloorGeometry::drawGeometry(QOpenGLShaderProgram *program, QOpenGLTexture *texture) {
}

void FloorGeometry::drawGeometry(QOpenGLShaderProgram *program) {
    program->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)nullptr);
}

void FloorGeometry::setupAttributePointer(QOpenGLShaderProgram *program) {
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

QVector<VertexData> FloorGeometry::getVerticesData() {
    vertices = {
            {QVector3D(5.0f, 0.0f,  5.0f),  QVector2D(1.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(-5.0f, 0.0f,  5.0f),  QVector2D(0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(-5.0f, 0.0f, -5.0f),  QVector2D(0.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)},

            {QVector3D(5.0f, 0.0f,  5.0f),  QVector2D(1.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(-5.0f, 0.0f, -5.0f),  QVector2D(0.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(5.0f, 0.0f, -5.0f),  QVector2D(1.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)}
    };

    return vertices;
}

QVector<GLuint> FloorGeometry::getIndices() {
    indices = {
            0, 1, 2, 3, 4, 5
    };
    return indices;
}
