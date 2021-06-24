#include "AxisSystem.h"
#include "Camera.h"

QVector <AxisData> AxisSystem::GetAxisData() {
    QVector <AxisData> data = {
            {QVector3D(1, 0, 0)}, {QVector3D(0, 0, 0)},
            {QVector3D(0, 1, 0)}, {QVector3D(0, 0, 0)},
            {QVector3D(0, 0, 1)}, {QVector3D(0, 0, 0)}
    };
    return data;
}

AxisSystem::AxisSystem():
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer)
{
    // buffer
    QOpenGLFunctions_4_5_Core::initializeOpenGLFunctions();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    vbo.create();
    ebo.create();

    // matrix
    mvpMatrix.setToIdentity();
}

AxisSystem::~AxisSystem() {
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
}


void AxisSystem::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(GetAxisData().constData(), GetAxisData().count() * sizeof(AxisData));

//    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
//    ebo.bind();
//    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void AxisSystem::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    int vertexLocation = program->attributeLocation("position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(AxisData));

    program->release();
}

void AxisSystem::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    program->setUniformValue("color", QVector4D(1, 0, 0, 1));
    glDrawArrays(GL_LINES, 0, 2);
    program->setUniformValue("color", QVector4D(0, 1, 0, 1));
    glDrawArrays(GL_LINES, 2, 2);
    program->setUniformValue("color", QVector4D(0, 0.5, 1, 1));
    glDrawArrays(GL_LINES, 4, 2);
}
