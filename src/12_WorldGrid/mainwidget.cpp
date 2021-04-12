#include "mainwidget.h"
#include <QKeyEvent>
#include <qmath.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]
#define RESOLUTION 2048

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          ContainerTexture(nullptr),
          cube_geometry(nullptr),
          InfiniteGrid(nullptr),
          camera(nullptr) {

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
}

void MainWidget::paintGL() {
    model.setToIdentity();
    model.translate(QVector3D(0, 0, 0));
    model.rotate(45,0,3,0);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            ContainerTexture);

    // Infinite Grid
    model.setToIdentity();
    SHADER(1)->setUniformValue("near_plane", 0.01f);
    SHADER(1)->setUniformValue("far_plane", 10000.0f);
    InfiniteGrid->drawGeometry(
            SHADER(1),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            ContainerTexture);
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SimpleObject.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/SimpleObject.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/12_WorldGrid/Shaders/WorldGrid.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/12_WorldGrid/Shaders/WorldGrid.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();
}

void MainWidget::initGeometry() {
    cube_geometry = new CubeGeometry;
    cube_geometry->initGeometry();
    cube_geometry->setupAttributePointer(SHADER(0));

    InfiniteGrid = new RectangleGeometry;
    InfiniteGrid->initGeometry();
    InfiniteGrid->setupAttributePointer(SHADER(1));
}

void MainWidget::initTexture() {
    auto imageData = QImage(QString("src/texture/container.jpg")).convertToFormat(QImage::Format_RGBA8888);

    ContainerTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    ContainerTexture->create();
    ContainerTexture->setSize(imageData.width(), imageData.height(), imageData.depth());
    ContainerTexture->setFormat(QOpenGLTexture::RGB32F);
    ContainerTexture->setMipLevels(ContainerTexture->maximumMipLevels());
    ContainerTexture->allocateStorage();
    ContainerTexture->bind();

    ContainerTexture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageData.constBits());
    ContainerTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    ContainerTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    ContainerTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    ContainerTexture->generateMipMaps();
}

void MainWidget::glSetting() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete ContainerTexture;
    delete cube_geometry;
    delete InfiniteGrid;

    camera = nullptr;
    ContainerTexture = nullptr;
    cube_geometry = nullptr;
    InfiniteGrid = nullptr;

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
        QPixmap cursorMap = QPixmap("F:/CLionProjects/QtReference/src/17_qopengl_mess/Camera/zoomIn_resize.png");
        QApplication::setOverrideCursor(cursorMap);

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
        QApplication::restoreOverrideCursor();
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
