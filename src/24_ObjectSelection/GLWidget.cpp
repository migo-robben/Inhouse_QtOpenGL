#include "GLWidget.h"
#include <QKeyEvent>

#define SHADER(x) programs[x]

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i = 0; i < 3; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 8.0);
    camera = std::make_shared<Camera>(cameraPos);
}

GLWidget::~GLWidget() {
    cleanup();
}

QSize GLWidget::minimumSizeHint() const {
    return {50, 50};
}

QSize GLWidget::sizeHint() const {
    return {540, 540};
}

void GLWidget::initializeGL() {
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);
    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    // ----- Initialization ----- //
    initShaders();
    initTexture();
    initGeometry();

    glSetting();
}

void GLWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void GLWidget::paintGL() {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // ----- Set Draw Mode ----- //
//    drawOutline();
    drawDefaultWithStencil();

    // ----- Selected object ----- //
    if (check)
        checkSelected();
}

void GLWidget::drawOutline() {
    // ----- Draw floor ------ //
    glStencilMask(0x00);
    model.setToIdentity();
    model.translate(0.0, -1.02, 0.0);
    floor->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            floorTex.get());

    // ----- Draw default cube ------ //
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    model.setToIdentity();
    model.translate(-1.5, -0.5, 0.0);
    model.rotate(45, QVector3D(0.0, 1.0, 0.0));
    model.scale(0.5);
    cube->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());

    model.setToIdentity();
    model.translate(1.0, 0.0, 0.0);
    model.rotate(-75, QVector3D(0.0, 1.0, 0.0));
    cube->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());

    // ----- Draw outline cube ------ //
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);

    model.setToIdentity();
    model.translate(-1.5, -0.5, 0.0);
    model.rotate(45, QVector3D(0.0, 1.0, 0.0));
    model.scale(0.515);
    cube->drawGeometry(
            SHADER(1),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());

    model.setToIdentity();
    model.translate(1.0, 0.0, 0.0);
    model.rotate(-75, QVector3D(0.0, 1.0, 0.0));
    model.scale(1.015);
    cube->drawGeometry(
            SHADER(1),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());

    // ----- Final ----- //
    glStencilMask(0xFF);
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::drawDefaultWithStencil() {
    int i = 1;

    glStencilFunc(GL_ALWAYS, i, 0xFF);
    model.setToIdentity();
    model.translate(0.0, -1.02, 0.0);
    floor->drawGeometry(
            selectedObjectIndex == i ? SHADER(2) : SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            floorTex.get());

    glStencilFunc(GL_ALWAYS, ++i, 0xFF);
    model.setToIdentity();
    model.translate(-1.5, -0.5, 0.0);
    model.rotate(45, QVector3D(0.0, 1.0, 0.0));
    model.scale(0.5);
    cube->drawGeometry(
            selectedObjectIndex == i ? SHADER(2) : SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());

    glStencilFunc(GL_ALWAYS, ++i, 0xFF);
    model.setToIdentity();
    model.translate(1.0, 0.0, 0.0);
    model.rotate(-75, QVector3D(0.0, 1.0, 0.0));
    cube->drawGeometry(
            selectedObjectIndex == i ? SHADER(2) : SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            cubeTex.get());
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    // init first shader program
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SimpleObject.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/SimpleObject.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SimpleObject.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/24_ObjectSelection/Shaders/outline.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();

    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/SimpleObject.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/24_ObjectSelection/Shaders/selectedObject.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
}

void GLWidget::initGeometry() {
    cube = std::make_shared<CubeGeometry>();
    cube->initGeometry();
    cube->setupAttributePointer(SHADER(0));

    floor = std::make_shared<FloorGeometry>();
    floor->initGeometry();
    floor->setupAttributePointer(SHADER(0));
}

void GLWidget::initTexture() {
    floorTex = std::make_shared<QOpenGLTexture>(
            QImage(QString("src/texture/metal.png")));
    cubeTex = std::make_shared<QOpenGLTexture>(
            QImage(QString("src/texture/container.jpg")));
}

void GLWidget::checkSelected() {
    delete mFBO;
    mFBO = nullptr;

    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    QOpenGLFramebufferObjectFormat format;
    format.setSamples(0);
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    mFBO = new QOpenGLFramebufferObject(frameBufferSize, format);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->defaultFramebufferObject());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO->handle());

    ctx->extraFunctions()->glBlitFramebuffer(0, 0, width(), height(), 0, 0, mFBO->width(), mFBO->height(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    mFBO->bind(); // must rebind, otherwise it won't work!
    glReadPixels(mouseSelectedPos.x(), width() - mouseSelectedPos.y() - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &selectedObjectIndex);

    if (selectedObjectIndex != 0)
        selectedObj = true;
    else
        selectedObj = false;

    mFBO->release();
    mouseSelectedPos = QPoint(0, 0);

    printf("Stencil index: %u, selected object: %s\n", selectedObjectIndex, selectedObj ? "true" : "false");

    check = false;
    update();
}

void GLWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();

    floorTex->destroy();
    cubeTex->destroy();

    doneCurrent();
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
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
    else if (event->buttons() == Qt::LeftButton) {
        // ----- Coordination ----- //
//        qDebug() << "Frame Geometry size:" << this->frameGeometry().size();
//        qDebug() << "Geometry size: " << this->geometry().size();
//        qDebug() << "Mouse click pos: " << mousePos.x() << mousePos.y();
//        qDebug() << "Window pos: " << pos();
//        qDebug() << "Screen pos: " << event->screenPos();

        mouseSelectedPos = event->pos();
        check = true;
        update();
    }
    QWidget::mousePressEvent(event);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (zoomInProcessing) {
        QApplication::restoreOverrideCursor();
        zoomInProcessing = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
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

void GLWidget::wheelEvent(QWheelEvent *event) {
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

#include "GLWidget.moc"
