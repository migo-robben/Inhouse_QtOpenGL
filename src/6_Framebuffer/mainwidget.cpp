#include "mainwidget.h"
#include <QKeyEvent>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          cubeGeometry(nullptr),
          gridGeometry(nullptr),
          containerTexture(nullptr),
          fbo(nullptr),
          justForDebug(false),
          camera(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<2; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 8.0);
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

    fbo = createFBOPointer();
}

void MainWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
}

void MainWidget::paintGL() {
    fbo->bind();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);

    model.setToIdentity();
    model.rotate(45,0,1,0);
    cubeGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            containerTexture);

    model.setToIdentity();
    model.translate(2.5, -0.5, 0);
    model.rotate(-15,0,1,0);
    model.scale(0.5f);
    cubeGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            containerTexture);

    model.setToIdentity();
    model.translate(0, -1.05, 0);
    model.rotate(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), -90));
    model.scale(5.0);
    gridGeometry->drawGeometry(SHADER(0),
                                 model,
                                 camera->getCameraView(),
                                 camera->getCameraProjection(),
                                 gridTexture);

    QOpenGLFramebufferObject::bindDefault();

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (justForDebug) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gridGeometry->drawGeometry(SHADER(1));
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        gridGeometry->drawGeometry(SHADER(1), fbo, false);
    }
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    if (fbo != nullptr) {
        delete fbo;
        fbo = nullptr;
    }
    fbo = createFBOPointer();

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    // init first shader program
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SimpleObject.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/SimpleObject.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();

    // init second shader program - for screen quad
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/ScreenPlane.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
}

void MainWidget::initGeometry() {
    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(0));

    gridGeometry = new GridGeometry;
    gridGeometry->initGeometry();
    gridGeometry->setupAttributePointer(SHADER(0));
}

void MainWidget::initTexture() {
    containerTexture = new QOpenGLTexture(QImage(QString("src/texture/container.jpg")));
    gridTexture = new QOpenGLTexture(QImage(QString("src/texture/metal.png")));
}

QOpenGLFramebufferObject* MainWidget::createFBOPointer() {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(QOpenGLTexture::RGB8_UNorm);
    format.setSamples(16);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    return new QOpenGLFramebufferObject(frameBufferSize, format);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete containerTexture;
    delete gridTexture;
    delete cubeGeometry;
    delete gridGeometry;
    delete camera;
    delete fbo;

    containerTexture = nullptr;
    gridTexture = nullptr;
    cubeGeometry = nullptr;
    gridGeometry = nullptr;
    camera = nullptr;
    fbo = nullptr;

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
    else if (event->buttons() == Qt::MiddleButton && event->modifiers() == Qt::AltModifier) {
        mousePos = event->pos();
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->localPos(),
                              event->screenPos(),
                              Qt::MiddleButton,
                              event->buttons() | Qt::MiddleButton,
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
    else if (event->buttons() == Qt::MiddleButton && event->modifiers() == Qt::AltModifier) {
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
    int offset = event->angleDelta().y() / 8;

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

void MainWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_D) {
        justForDebug = !justForDebug;
        update();
    }
    QWidget::keyPressEvent(event);
}