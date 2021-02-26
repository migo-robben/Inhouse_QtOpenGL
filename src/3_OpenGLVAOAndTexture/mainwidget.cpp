#include "mainwidget.h"


MainWidget::MainWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      vbo(QOpenGLBuffer::VertexBuffer),
      ebo(QOpenGLBuffer::IndexBuffer) {
}

MainWidget::~MainWidget() {
    makeCurrent();

    // delete all buffers
    vbo.destroy();
    ebo.destroy();

    doneCurrent();
}

QSize MainWidget::sizeHint() const {
    return {640, 640};
}

void MainWidget::initializeGL() {
    initializeOpenGLFunctions();

    // Init Shaders
    initShaders();

    // Init Texture
    initTextures();

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
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0.0, -2);

    // Set model-view-projection matrix
    program.setUniformValue("mvp_matrix", projection * matrix);

    // bind texture
    program.setUniformValue("map", 0);
    texture->bind();

    // Draw plane geometry
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void MainWidget::initShaders() {
    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, "src\\3_OpenGLVAOAndTexture\\Plane.vs.glsl"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, "src\\3_OpenGLVAOAndTexture\\Plane.fs.glsl"))
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
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Generate buffers
    vbo.create();
    ebo.create();

    // Setting and allocate memory to buffers
    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), 4 * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), 6 * sizeof(GLushort));

    program.bind();
    // Offset for position
    quintptr offset = 0;
    vertexLocation = program.attributeLocation("a_position");
    colorLocation = program.attributeLocation("a_color");
    coordLocation = program.attributeLocation("a_coord");

    // Tell OpenGL programmable pipeline how to locate vertex position data
    program.enableAttributeArray(vertexLocation);
    program.setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for color data
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    program.enableAttributeArray(colorLocation);
    program.setAttributeBuffer(colorLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coord
    offset += sizeof(QVector3D);

    program.enableAttributeArray(coordLocation);
    program.setAttributeBuffer(coordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    program.release();
}

void MainWidget::initTextures() {
    texture = new QOpenGLTexture(QImage(QString("src\\3_OpenGLVAOAndTexture\\texture\\wall.jpg")));
}

QVector<VertexData> MainWidget::getVerticesData() {
    vertices = {
            {QVector3D(0.5f, 0.5f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector2D(1.0f, 1.0f)},
            {QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector2D(0.0f, 0.0f)},
            {QVector3D(0.5f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector2D(1.0f, 0.0f)},
            {QVector3D(-0.5f, 0.5f, 0.0f), QVector3D(1.0f, 1.0f, 0.0f), QVector2D(0.0f, 1.0f)}
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
