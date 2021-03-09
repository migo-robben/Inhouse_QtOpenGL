#include "mainwidget.h"
#include <QKeyEvent>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          customGeometry(nullptr),
          camera(nullptr),
          room_texture(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<1; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 3);
    camera = new Camera(cameraPos);
}

MainWidget::~MainWidget() {
    cleanup();
}

QSize MainWidget::minimumSizeHint() const {
    return {50, 50};
}

QSize MainWidget::sizeHint() const {
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
    model.translate(0,-0.3,0);
    model.rotate(-90,1,0,0);
    model.rotate(-135,0,0,1);
    customGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            room_texture);
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/CustomGeometry.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/CustomGeometry.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();
}

void MainWidget::initGeometry() {
    customGeometry = new CustomGeometry(QString("src/resource/viking_room.obj"));

    customGeometry->initGeometry();
    customGeometry->setupAttributePointer(SHADER(0));
}

void MainWidget::initTexture() {
    room_texture = new QOpenGLTexture(QImage(QString("src/resource/viking_room.png")));
}

void MainWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
}

void MainWidget::cleanup() {
    makeCurrent();

    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete customGeometry;
    delete room_texture;

    camera = nullptr;
    customGeometry = nullptr;
    room_texture = nullptr;

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
    camera->setCameraPerspective(aspect);
    update();

    QWidget::wheelEvent(event);
}