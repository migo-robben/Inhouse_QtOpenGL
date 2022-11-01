#include "hellowindow.h"
#include <QKeyEvent>

HelloWindow::HelloWindow() : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    setFormat(format);

    setGeometry(QRect(10, 10, 640, 480));

    m_program = new QOpenGLShaderProgram(this);

    QVector3D cameraPos(0.0, 0.0, 7.0);
    camera = new Camera(cameraPos);
}

HelloWindow::~HelloWindow() {
    makeCurrent();

    delete m_program;
    m_program = nullptr;

    delete geometry;
    geometry = nullptr;

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

    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(0.0,0.0,0.0);
    geometry->drawGeometry(
            m_program,
            model,
            camera->getCameraView(),
            camera->projection);
}

void HelloWindow::initShaders() {
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(
            "attribute highp vec3 aPos;"
            "attribute mediump vec3 aNormal;"
            "attribute mediump vec2 aCoord;"
            "uniform mediump mat4 model;"
            "uniform mediump mat4 view;"
            "uniform mediump mat4 projection;"
            "varying mediump vec3 color;"
            "varying highp vec2 coord;"
            "void main() {"
            "   gl_Position = projection * view * model * vec4(aPos, 1.0);"
            "   color = aNormal;"
            "   coord = aCoord;"
            "}");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(
            "varying mediump vec3 color;"
            "varying highp vec2 coord;"
            "void main() {"
            "    gl_FragColor = vec4(color, 1.0);"
            "}");

    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();
    m_program->bind();
    m_program->release();
}

void HelloWindow::initTextures() {
}

void HelloWindow::initGeometry() {
    geometry = new CustomGeometry(QString(":/model/rubbertoy.obj"));
    geometry->initGeometry();
    geometry->setupAttributePointer(m_program);
}

#include "Helper/mouseEventTemplate.h"
