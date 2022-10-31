#include "hellowindow.h"
#include <QKeyEvent>

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
    texture = nullptr;

    QVector3D cameraPos(0.0, 0.0, 7.0);
    camera = new Camera(cameraPos);
}

HelloWindow::~HelloWindow() {
    makeCurrent();

    delete m_program;
    m_program = nullptr;

    delete texture;
    texture = nullptr;

    // delete all buffers
    arrayBuf.destroy();
    indexBuf.destroy();

    doneCurrent();
}

void HelloWindow::initializeGL() {
#ifndef __EMSCRIPTEN__
    initializeOpenGLFunctions();
#endif
    glClearColor(0.1f, 0.1f, 0.1f, 1.0);

    initShaders();
    initTextures();
    initGeometry();

    glSetting();
}

void HelloWindow::glSetting() {
    glEnable(GL_DEPTH_TEST);
    // Enable back face culling
    glEnable(GL_CULL_FACE);
}

void HelloWindow::resizeGL(int w, int h) {
    qreal aspect = qreal(w) / qreal(h ? h : 1);
    camera->setCameraPerspective(aspect);
}

void HelloWindow::paintGL() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Binding program
    m_program->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0f, 0.0f, 0.0f);
    matrix.rotate(rotation);

    // Set model-view-projection matrix
    m_program->setUniformValue("mvp_matrix", camera->projection * camera->getCameraView() * matrix);

    // bind texture
    m_program->setUniformValue("uSampler", 0);
    texture->bind();

    // Draw geometry
    glDrawElements(GL_TRIANGLE_STRIP, getIndices().count(), GL_UNSIGNED_SHORT, (void*)nullptr);

    m_program->release();
}

void HelloWindow::initShaders() {
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(
            "attribute highp vec3 a_position;"
            "attribute mediump vec3 a_normal;"
            "attribute mediump vec2 a_coord;"
            "uniform mediump mat4 mvp_matrix;"
            "varying mediump vec3 color;"
            "varying highp vec2 coord;"
            "void main() {"
            "   gl_Position = mvp_matrix * vec4(a_position, 1.0);"
            "   color = a_normal;"
            "   coord = a_coord;"
            "}");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(
            "varying mediump vec3 color;"
            "varying highp vec2 coord;"
            "uniform sampler2D uSampler;"
            "void main() {"
            "    gl_FragColor = texture2D(uSampler, coord);"
            "}");

    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();
    m_program->bind();
    m_program->release();
}

void HelloWindow::initTextures() {
    texture = new QOpenGLTexture(QImage(QString(":/images/container.jpg")).mirrored());
}

void HelloWindow::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Generate buffers
    arrayBuf.create();
    indexBuf.create();

    // Setting and allocate memory to buffers
    arrayBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    arrayBuf.bind();
    arrayBuf.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    indexBuf.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuf.bind();
    indexBuf.allocate(getIndices().constData(), getIndices().count() * sizeof(GLushort));

    m_program->bind();

    // Offset for position
    int offset = 0;
    vertexLocation = m_program->attributeLocation("a_position");
    coordLocation = m_program->attributeLocation("a_coord");
    normalLocation = m_program->attributeLocation("a_normal");

    // Tell OpenGL programmable pipeline how to locate vertex position data
    m_program->enableAttributeArray(vertexLocation);
    m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for color data
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex color data
    m_program->enableAttributeArray(coordLocation);
    m_program->setAttributeBuffer(coordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Offset for texture coord
    offset += sizeof(QVector2D);

    m_program->enableAttributeArray(normalLocation);
    m_program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    m_program->release();
}

QVector<VertexData> HelloWindow::getVerticesData() {
    vertices = {
            // Vertex data for face 0
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)},  // v0
            {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(1.0f, 0.0f),  QVector3D(0.0f, 0.0f, 1.0f)}, // v1
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 1.0f),  QVector3D(0.0f, 0.0f, 1.0f)},  // v2
            {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(1.0f, 1.0f),  QVector3D(0.0f, 0.0f, 1.0f)}, // v3

            // Vertex data for face 1
            {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D( 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)}, // v4
            {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)}, // v5
            {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.0f, 1.0f), QVector3D(1.0f, 0.0f, 0.0f)},  // v6
            {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f), QVector3D(1.0f, 0.0f, 0.0f)}, // v7

            // Vertex data for face 2
            {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f)}, // v8
            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f)},  // v9
            {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.0f, 1.0f), QVector3D(0.0f, 0.0f, -1.0f)}, // v10
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f), QVector3D(0.0f, 0.0f, -1.0f)},  // v11

            // Vertex data for face 3
            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f)}, // v12
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(1.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f)},  // v13
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 1.0f), QVector3D(-1.0f, 0.0f, 0.0f)}, // v14
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(1.0f, 1.0f), QVector3D(-1.0f, 0.0f, 0.0f)},  // v15

            // Vertex data for face 4
            {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f)}, // v16
            {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f)}, // v17
            {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 1.0f), QVector3D(0.0f, -1.0f, 0.0f)}, // v18
            {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(1.0f, 1.0f), QVector3D(0.0f, -1.0f, 0.0f)}, // v19

            // Vertex data for face 5
            {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)}, // v20
            {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(1.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)}, // v21
            {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)}, // v22
            {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)}  // v23
    };

    return vertices;
}

QVector<GLushort> HelloWindow::getIndices() {
    indices = {
            0,  1,  2,  3,  3,     // Face 0 - triangle strip ( v0,  v1,  v2,  v3)
            4,  4,  5,  6,  7,  7, // Face 1 - triangle strip ( v4,  v5,  v6,  v7)
            8,  8,  9, 10, 11, 11, // Face 2 - triangle strip ( v8,  v9, v10, v11)
            12, 12, 13, 14, 15, 15, // Face 3 - triangle strip (v12, v13, v14, v15)
            16, 16, 17, 18, 19, 19, // Face 4 - triangle strip (v16, v17, v18, v19)
            20, 20, 21, 22, 23      // Face 5 - triangle strip (v20, v21, v22, v23)
    };
    return indices;
}

#include "Helper/mouseEventTemplate.h"
