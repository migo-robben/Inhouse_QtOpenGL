#include "GLWidget.h"
#include <QKeyEvent>
#include <QTimer>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          customGeometry(nullptr),
          camera(nullptr),
          diffuseTexture(nullptr),
          pbo(QOpenGLBuffer::PixelPackBuffer){

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<2; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 5);
    camera = new Camera(cameraPos);
}

void GLWidget::updateFrame() {
    update();
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

    // ----- add shader from source file/code ----- //
    initShaders();
    initTexture();
    // --------------------------------------- //

    glSetting();
    initGeometry();

    createBlendShapeTexBuffer();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GLWidget::updateFrame);
    timer->start(33);
    elapsedTimer.start();
}

void GLWidget::paintGL() {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float currentTime = elapsedTimer.elapsed() / 1000.0;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    customGeometry->animator.updateAnimation(deltaTime);
    QVector<QMatrix4x4> transforms = customGeometry->animator.getPoseTransforms();

    SHADER(0)->bind();
    SHADER(0)->setUniformValueArray("finalBonesMatrices", transforms.data(), transforms.size());
    SHADER(0)->setUniformValue("BlendShapeWeight1", customGeometry->animator.bsWeight1);
    SHADER(0)->setUniformValue("BlendShapeWeight2", customGeometry->animator.bsWeight2);
    SHADER(0)->setUniformValue("BlendShapeWeight3", customGeometry->animator.bsWeight3);
    SHADER(0)->setUniformValue("BlendShapeWeight4", customGeometry->animator.bsWeight4);
    SHADER(0)->setUniformValue("BlendShapeNum", customGeometry->m_NumBlendShape);

//    pbo.bind();
//    glActiveTexture(GL_TEXTURE0);
//    SHADER(0)->setUniformValue("blendShapeMap1", 0);
//    blendShapeTexture->bind();

    model.setToIdentity();
    model.translate(QVector3D(0.0, -1.0, 0.0));
    model.scale(0.1);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    customGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection(),
            diffuseTexture);
}

void GLWidget::createBlendShapeTexBuffer() {
//    int width, height, channels;
//    stbi_set_flip_vertically_on_load(true);
//    float *image = stbi_loadf("src/20_SkeletalAnimation/resource/vampire/textures/Pure.png", &width, &height, &channels, 0);

//    blendShapeTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
//    blendShapeTexture->setSize(512, 1);
//    blendShapeTexture->setFormat(QOpenGLTexture::RGB32F);
//    blendShapeTexture->create();
//    blendShapeTexture->bind();
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 512, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//    pbo.create();
//    pbo.bind();
//    QVector<float> data;
//    for (int i = 0; i < 512 * 1; i++)
//    {
//        data.append(255);  // R
//        data.append(0);  // G
//        data.append(0);    // B
//        data.append(255);  // A
//    }
//    data.resize(512);
//    data.fill(0.5);
//    pbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
//    pbo.allocate(data.constData(), data.count() * sizeof(GLubyte));
//    m_pixelData = (GLubyte*) pbo.map(QOpenGLBuffer::WriteOnly);

//    blendShapeTexture->setData(0, 0, QOpenGLTexture::RGB, QOpenGLTexture::Float32, data.data());
//    blendShapeTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
//    blendShapeTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
//    blendShapeTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

//    stbi_image_free(image);
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/20_SkeletalAnimation/shaders/skeletalAnimation.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/20_SkeletalAnimation/shaders/skeletalAnimation.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/20_SkeletalAnimation/shaders/blendShape.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/20_SkeletalAnimation/shaders/blendShape.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();
}

void GLWidget::initGeometry() {
    customGeometry = new CustomGeometry(QString("src/20_SkeletalAnimation/resource/testBlendShape.fbx")); // dancing_vampire
    customGeometry->initGeometry();
    customGeometry->initAnimation();
    customGeometry->initAnimator();
    customGeometry->setupAttributePointer(SHADER(0));
}

void GLWidget::initTexture() {
    diffuseTexture = new QOpenGLTexture(QImage(QString("src/20_SkeletalAnimation/resource/vampire/textures/Pure.png")));
}

void GLWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::cleanup() {
    makeCurrent();

    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete customGeometry;
    delete diffuseTexture;

    camera = nullptr;
    customGeometry = nullptr;
    diffuseTexture = nullptr;

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