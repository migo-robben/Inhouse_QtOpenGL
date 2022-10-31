#include "hellowindow.h"

HelloWindow::HelloWindow() : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate),
                             arrayBuf(QOpenGLBuffer::VertexBuffer),
                             indexBuf(QOpenGLBuffer::IndexBuffer)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    setFormat(format);

    setGeometry(QRect(10, 10, 640, 480));

    m_program = new QOpenGLShaderProgram(this);
}

HelloWindow::~HelloWindow() {
    delete m_program;
    m_program = nullptr;

    // delete all buffers
    arrayBuf.destroy();
    indexBuf.destroy();
}

void HelloWindow::initializeGL() {
#ifndef __EMSCRIPTEN__
    initializeOpenGLFunctions();
#endif
    glClearColor(0.1f, 0.1f, 0.2f, 1.0);

    initShaders();
    initGeometry();
}

void HelloWindow::resizeGL(int w, int h) {
    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const float zNear = 0.0, zFar = 7.0, fov = 45.0;
    // Reset projection
    projection.setToIdentity();
    // Set perspective projection
    projection.perspective(fov, float(w)/float(h), zNear, zFar);
}

void HelloWindow::paintGL() {
    glClearColor(0.1f, 0.1f, 0.2f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Binding program
    m_program->bind();

    // Binding buffers
    arrayBuf.bind();
    indexBuf.bind();

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0f, 0.0f, -2.0f);

    // Set model-view-projection matrix
    m_program->setUniformValue("mvp_matrix", projection * matrix);

    // Draw geometry
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, (void*)nullptr);

    m_program->release();
}

void HelloWindow::initShaders() {
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(
            "attribute highp vec3 a_position;"
            "attribute mediump vec3 a_color;"
            "uniform mediump mat4 mvp_matrix;"
            "varying mediump vec3 color;"
            "void main() {"
            "   gl_Position = mvp_matrix * vec4(a_position, 1.0);"
            "   color = a_color;"
            "}");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(
            "varying mediump vec3 color;"
            "void main() {"
            "    gl_FragColor = vec4(color, 1.0);"
            "}");

    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();
    m_program->bind();
    m_program->release();
}

void HelloWindow::initGeometry() {
    // Generate buffers
    arrayBuf.create();
    indexBuf.create();

    // Setting and allocate memory to buffers
    arrayBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    arrayBuf.bind();
    arrayBuf.allocate(getVerticesData().constData(), 3 * sizeof(VertexData));

    indexBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuf.bind();
    indexBuf.allocate(getIndices().constData(), 3 * sizeof(GLushort));

    m_program->bind();

    // Offset for position
    int offset = 0;
    vertexLocation = m_program->attributeLocation("a_position");
    colorLocation = m_program->attributeLocation("a_color");

    // Tell OpenGL programmable pipeline how to locate vertex position data
    m_program->enableAttributeArray(vertexLocation);
    m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for color data
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    m_program->enableAttributeArray(colorLocation);
    m_program->setAttributeBuffer(colorLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    m_program->release();
    arrayBuf.release();
    indexBuf.release();
}

QVector<VertexData> HelloWindow::getVerticesData() {
    vertices = {
            {QVector3D(0.0f, 0.5f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)},
            {QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
            {QVector3D(0.5f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)}
    };
    return vertices;
}

QVector<GLushort> HelloWindow::getIndices() {
    indices = {
            0, 1, 2
    };
    return indices;
}