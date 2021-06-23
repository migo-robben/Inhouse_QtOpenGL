#include "mainwidget.h"
#include <QKeyEvent>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          camera(nullptr),
          uiohcfnfa(nullptr),
          AlbedoMap(nullptr),
          NormalMap(nullptr),
          SpecularMap(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<4; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 3);
    camera = new Camera(cameraPos);
}

MainWidget::~MainWidget() {
    cleanup();
}

QSize MainWidget::minimumSizeHint() const {
    return {120, 120};
}

QSize MainWidget::sizeHint() const {
    camera->setScreenSize(QSize(540, 540));
    return {540, 540};
}

void MainWidget::initializeGL() {
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &MainWidget::cleanup);

    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    // ----- add shader from source file/code ----- //
    initShaders();
    initTexture();
    // --------------------------------------- //

    glSetting();
    initGeometry();
}

void MainWidget::paintGL() {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    model.setToIdentity();

    // draw uiohcfnfa
    model.rotate(-30, 0, 1, 0);
    SHADER(0)->bind();
    SHADER(0)->setUniformValue("lightPos", QVector3D(0.0f, 1.0f, 3.5f));
    SHADER(0)->setUniformValue("viewPos", camera->getCameraPosition());
    SHADER(0)->setUniformValue("diffuseWeight", 0.6f);
    SHADER(0)->setUniformValue("specularWeight", 0.75f);

    glActiveTexture(GL_TEXTURE0);
    SHADER(0)->setUniformValue("AlbedoMap", 0);
    AlbedoMap->bind();

    glActiveTexture(GL_TEXTURE1);
    SHADER(0)->setUniformValue("NormalMap", 1);
    NormalMap->bind();

    glActiveTexture(GL_TEXTURE2);
    SHADER(0)->setUniformValue("SpecularMap", 2);
    SpecularMap->bind();

//    uiohcfnfa->drawGeometry(
//            SHADER(0),
//            model,
//            camera->getCameraView(),
//            camera->getCameraProjection());

//    SHADER(1)->bind();
//    uiohcfnfa->drawGeometry(
//            SHADER(1),
//            model,
//            camera->getCameraView(),
//            camera->getCameraProjection(),
//            AlbedoMap);

    QMatrix4x4 sphere_trans;
    sphere_trans.setToIdentity();
    sphere_trans.translate(2.5, 0, 0);
    sphereGeometry->drawGeometry(
            SHADER(3),model,camera->getCameraView(),camera->getCameraProjection());
    sphereGeometry->drawGeometry(
            SHADER(3),model*sphere_trans,camera->getCameraView(),camera->getCameraProjection());
    model.setToIdentity();
    glViewport(-10, -10, 80, 80);
    axisSystem->drawGeometry(SHADER(2), model, camera->getCameraView(),camera->getCameraProjection(), camera);
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect, width, height);
    QSize screen_size(width, height);
    camera->setScreenSize(screen_size);
    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/NormalMapping.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/NormalMapping.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/CustomGeometry.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/CustomGeometry.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/Axis.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/Axis.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();

    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SkySphere.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/SkySphere.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
    if (!SHADER(3)->bind())
        close();
}

void MainWidget::initGeometry() {
    uiohcfnfa = new CustomGeometry(QString("src/resource/uiohcfnfa/uiohcfnfa.fbx"));
    uiohcfnfa->initGeometry();
    uiohcfnfa->setupAttributePointer(SHADER(0));

    axisSystem = new AxisSystem();
    axisSystem->initGeometry();
    axisSystem->setupAttributePointer(SHADER(2));

    sphereGeometry = new SphereGeometry();
    sphereGeometry->initGeometry();
    sphereGeometry->setupAttributePointer(SHADER(3));
}

void MainWidget::initTexture() {
    AlbedoMap = new QOpenGLTexture(QImage(QString("src/resource/uiohcfnfa/uiohcfnfa_Albedo.jpg")));
    NormalMap = new QOpenGLTexture(QImage(QString("src/resource/uiohcfnfa/uiohcfnfa_Normal.jpg")));
    SpecularMap = new QOpenGLTexture(QImage(QString("src/resource/uiohcfnfa/uiohcfnfa_Specular.jpg")));
}

void MainWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();

    delete camera;
    delete uiohcfnfa;
    delete AlbedoMap;
    delete NormalMap;
    delete SpecularMap;

    camera = nullptr;
    uiohcfnfa = nullptr;
    AlbedoMap = nullptr;
    NormalMap = nullptr;
    SpecularMap = nullptr;

    doneCurrent();
}

void MainWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        mousePos = event->pos();
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->localPos(),
                              event->screenPos(),
                              Qt::LeftButton,
                              event->buttons() | Qt::LeftButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    else if (event->button() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        mousePos = event->pos();

        zoomInProcessing = true;

        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->localPos(),
                              event->screenPos(),
                              Qt::RightButton,
                              event->buttons() | Qt::RightButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    else if (event->buttons() == Qt::MidButton && event->modifiers() == Qt::AltModifier) {
        mousePos = event->pos();
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->localPos(),
                              event->screenPos(),
                              Qt::MidButton,
                              event->buttons() | Qt::MidButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    QWidget::mousePressEvent(event);
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (zoomInProcessing) {
        zoomInProcessing = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void MainWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraRotateEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraZoomEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::MidButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraTranslateEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else {
        QWidget::mouseMoveEvent(event);
    }
}

void MainWidget::wheelEvent(QWheelEvent *event) {
    int offset = event->delta() / 8;

    qreal fov = camera->getCameraFov();
    fov += (float)-offset * camera->MouseWheelSensitivity;
    if (fov < camera->fovLowerBound) {
        fov = camera->fovLowerBound;
    }
    else if (fov > camera->fovUpperBound) {
        fov = camera->fovUpperBound;
    }
    camera->setCameraFov(fov);
    qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
    camera->setCameraPerspective(aspect, width(), height());
    update();

    QWidget::wheelEvent(event);
}
