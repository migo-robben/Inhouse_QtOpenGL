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

void AxisSystem::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection, Camera* camera) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    program->bind();
    QVector3D aim_pos = camera->getCameraAimAt();
    QVector3D camera_pos = camera->getCameraPosition();
    qreal length = QVector3D(camera_pos-aim_pos).length();
    qreal fov = camera->getCameraFov();
    QMatrix4x4 ortho = camera->getOrthoCamera();
    qreal near_clip = camera->getCameraNearClipPlane();
    qreal far_clip = camera->getCameraFarClipPlane();
    qreal aspect = camera->getCameraAspect();
    QSize screen_size = camera->getScreenSize();
    qreal radio_x = screen_size.width() / 500.0;
    qreal radio_y = screen_size.height() / 500.0;

    qreal theta1 = qDegreesToRadians(45.0/2);
    qreal theta2 = qDegreesToRadians(45.0*1.0/2);
    qreal move1 = 2 / qCos(theta1) * qSin(theta1);
    qreal move2 = 2 / qCos(theta2) * qSin(theta2);

    model.translate(aim_pos);
    model.scale(0.6);  // set axis scale
    view.setRow(2, QVector4D(view(2, 0),
                             view(2, 1),
                             view(2, 2),
                             -3.0f));  // set axis scale with camera
    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", ortho);

    program->setUniformValue("color", QVector4D(1, 0, 0, 1));
    glDrawArrays(GL_LINES, 0, 2);
    program->setUniformValue("color", QVector4D(0, 1, 0, 1));
    glDrawArrays(GL_LINES, 2, 2);
    program->setUniformValue("color", QVector4D(0, 0, 1, 1));
    glDrawArrays(GL_LINES, 4, 2);
}
