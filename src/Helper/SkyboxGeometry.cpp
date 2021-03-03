#include "SkyboxGeometry.h"

void SkyboxGeometry::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void SkyboxGeometry::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Offset for position
    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    program->release();
}

void SkyboxGeometry::drawGeometry(QOpenGLShaderProgram *program,
        QMatrix4x4 model,
        QMatrix4x4 view,
        QMatrix4x4 projection,
        QOpenGLTexture *texture) {
    drawGeometry(program, view, projection, texture);
}

void SkyboxGeometry::drawGeometry(QOpenGLShaderProgram *program,
        QMatrix4x4 view,
        QMatrix4x4 projection,
        QOpenGLTexture *texture) {
    program->bind();

    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);
    program->setUniformValue("cubeMap", 0);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    texture->bind();
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureId());
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

QVector<VertexData> SkyboxGeometry::getVerticesData() {
    vertices = {
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},

            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},

            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},

            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},

            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 0.0f)},

            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)}
    };
    return vertices;
}

QVector<GLuint> SkyboxGeometry::getIndices() {
    indices = {
            0,1,2,3,4,5,
            6,7,8,9,10,11,
            12,13,14,15,16,17,
            18,19,20,21,22,23,
            24,25,26,27,28,29,
            30,31,32,33,34,35
    };
    return indices;
}
