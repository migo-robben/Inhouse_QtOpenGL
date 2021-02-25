#include "mainwidget.h"


MainWidget::MainWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    arrayBuf(QOpenGLBuffer::VertexBuffer),
    indexBuf(QOpenGLBuffer::IndexBuffer) {
}

MainWidget::~MainWidget() {
    makeCurrent();

    // delete all buffers
    arrayBuf.destroy();
    indexBuf.destroy();

    doneCurrent();
}

QSize MainWidget::sizeHint() const {
    return {640, 640};
}

void MainWidget::initializeGL() {
    initializeOpenGLFunctions();

    // Init Shaders
    initShaders();

    // Init Geometry
    initPlaneGeometry();
}

void MainWidget::resizeGL(int w, int h) {
    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 0.0, zFar = 7.0, fov = 45.0;
    // Reset projection
    projection.setToIdentity();
    // Set perspective projection
    projection.perspective(fov, qreal(w)/qreal(h), zNear, zFar);
}

void MainWidget::paintGL() {
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.15, 0.15, 0.15, 1);

    // Binding program
    program.bind();
    // Binding buffers
    arrayBuf.bind();
    indexBuf.bind();

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0.0, -2);
    rotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 0) * rotation;
    matrix.rotate(rotation);

    // Set model-view-projection matrix
    program.setUniformValue("mvp_matrix", projection * matrix);

    // Draw plane geometry
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void MainWidget::initShaders() {
    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, "src\\2_OpenGLWidget\\Plane.vs.glsl"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, "src\\2_OpenGLWidget\\Plane.fs.glsl"))
        close();

    // Link shader pipeline
    if (!program.link())
        close();

    // Bind shader pipeline for use
    if (!program.bind())
        close();

    program.release();
}

void MainWidget::initPlaneGeometry() {

    // Generate buffers
    arrayBuf.create();
    indexBuf.create();

    // Setting and allocate memory to buffers
    arrayBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    arrayBuf.bind();
    arrayBuf.allocate(getVerticesData().constData(), 4 * sizeof(VertexData));

    indexBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuf.bind();
    indexBuf.allocate(getIndices().constData(), 6 * sizeof(GLushort));

    program.bind();
    // Offset for position
    quintptr offset = 0;
    vertexLocation = program.attributeLocation("a_position");
    colorLocation = program.attributeLocation("a_color");

    // Tell OpenGL programmable pipeline how to locate vertex position data
    program.enableAttributeArray(vertexLocation);
    program.setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for color data
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    program.enableAttributeArray(colorLocation);
    program.setAttributeBuffer(colorLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    program.release();
    arrayBuf.release();
    indexBuf.release();
}

QVector<VertexData> MainWidget::getVerticesData() {
    vertices = {
            {QVector3D(0.5f, 0.5f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)},
            {QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(0.5f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)},
            {QVector3D(-0.5f, 0.5f, 0.0f), QVector3D(1.0f, 0.5f, 0.2f)},
    };

    return vertices;
}

QVector<GLushort> MainWidget::getIndices() {
    indices = {
            0, 1, 2,
            1, 0, 3
    };

    return indices;
}
