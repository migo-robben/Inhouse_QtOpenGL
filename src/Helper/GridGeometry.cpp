#include "GridGeometry.h"

void GridGeometry::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void GridGeometry::setupAttributePointer(QOpenGLShaderProgram *program) {
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

    program->release();
}

QVector<VertexData> GridGeometry::getVerticesData() {
    vertices = {
            { QVector3D(1.0f, 1.0f, 0.0f), QVector2D(1.0f, 1.0f) },
            { QVector3D(-1.0f, -1.0f, 0.0f), QVector2D(0.0f, 0.0f) },
            { QVector3D(1.0f, -1.0f, 0.0f), QVector2D(1.0f, 0.0f) },
            { QVector3D(-1.0f, 1.0f, 0.0f), QVector2D(0.0f, 1.0f) },
    };

    return vertices;
}

QVector<GLuint> GridGeometry::getIndices() {
    indices = {
            0, 1, 2,
            1, 0, 3
    };
    return indices;
}

void
GridGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection,
                                QOpenGLTexture *texture) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);
    program->setUniformValue("map", 0);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    texture->bind();
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

void GridGeometry::drawGeometry(QOpenGLShaderProgram *program) {
    program->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

void GridGeometry::drawGeometry(QOpenGLShaderProgram *program, QOpenGLFramebufferObject *fbo, bool saveToDisk) {
    if (saveToDisk)
        fbo->toImage().save(QString("RenderOut/drawScene.png"));

    program->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    GLuint texture;
    QScopedPointer<QOpenGLFramebufferObject> tmpFBO;

    if (fbo->format().samples() > 0) {
        tmpFBO.reset(new QOpenGLFramebufferObject(fbo->size()));
        QOpenGLFramebufferObject::blitFramebuffer(tmpFBO.data(), fbo);
        texture = tmpFBO->takeTexture(0);

        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, fbo->takeTexture(0));
        glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
    }
}