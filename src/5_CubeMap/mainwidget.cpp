#include "mainwidget.h"
#include <QKeyEvent>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          skyboxGeometry(nullptr),
          cubeGeometry(nullptr),
          camera(nullptr),
          skyboxTexture(nullptr),
          containerTexture(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<2; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 8);
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
    model.rotate(45,0,1,0);
    cubeGeometry->drawGeometry(
            SHADER(1),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            containerTexture);

    // draw cube box
    // 系统的最大深度为1，又因为skybox shader设置了box的深度为1，所以要修改一下深度测试方程
    // 使得box的深度小于或这等于系统的最大深度也能通过深度测试
    glDepthFunc(GL_LEQUAL);
    skyboxGeometry->drawGeometry(
            SHADER(0),
            camera->getCameraView(),
            camera->getCameraProjection(),
            skyboxTexture);
    glDepthFunc(GL_LESS);
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    // for cube box
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/Skybox.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/Skybox.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/Cube.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/Cube.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();
}

void MainWidget::initGeometry() {
    skyboxGeometry = new SkyboxGeometry;
    skyboxGeometry->initGeometry();
    skyboxGeometry->setupAttributePointer(SHADER(0));

    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(1));
}

void MainWidget::initTexture() {
    loadCubeMap();

    containerTexture = new QOpenGLTexture(QImage(QString("src/texture/container.jpg")));
    containerTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    containerTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    containerTexture->setMagnificationFilter(QOpenGLTexture::Linear);
}

void MainWidget::loadCubeMap() {
    const QImage tempImage = QImage(faces.at(0)).convertToFormat(QImage::Format_RGBA8888);

    skyboxTexture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    skyboxTexture->create();
    skyboxTexture->setSize(tempImage.width(), tempImage.height(), tempImage.depth());
    skyboxTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    skyboxTexture->setMipLevels(skyboxTexture->maximumMipLevels());
    skyboxTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

    for (int i=0; i<6; i++) {
        const QImage imageData = QImage(faces.at(i)).convertToFormat(QImage::Format_RGBA8888);
        skyboxTexture->setData(
                0,
                0,
                static_cast<QOpenGLTexture::CubeMapFace>(QOpenGLTexture::CubeMapPositiveX + i),
                QOpenGLTexture::RGBA,
                QOpenGLTexture::UInt8,
                imageData.constBits(),
                Q_NULLPTR);
    }

    skyboxTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    skyboxTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    skyboxTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    skyboxTexture->generateMipMaps();
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
    delete skyboxTexture;
    delete containerTexture;
    delete cubeGeometry;
    delete skyboxGeometry;

    camera = nullptr;
    skyboxTexture = nullptr;
    containerTexture = nullptr;
    cubeGeometry = nullptr;
    skyboxGeometry = nullptr;

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