#include "GLWidget.h"
#include <QRandomGenerator>
#include <QKeyEvent>

#define SHADER(x) programs[x]

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          customGeometry(nullptr),
          rectGeometry(nullptr),
          camera(nullptr),
          roomTexture(nullptr),
          gBufferFBO(nullptr),
          ssaoFBO(nullptr),
          ssaoBlurFBO(nullptr),
          gPositionDepth(nullptr),
          gNormal(nullptr),
          gAlbedo(nullptr),
          mask(nullptr),
          ssaoColorBuffer(nullptr),
          ssaoColorBufferBlur(nullptr),
          noiseTexture(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<6; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 5);
    camera = new Camera(cameraPos);
    camera->setCameraNearClipPlane(0.1f);
    camera->setCameraFarClipPlane(50.0f);

    gBufferTextures.clear();
    gBufferTextures.resize(4);
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

    generateGBufferTexture(width());
    generateSSAOColorBufferTexture(width());
    generateSSAOColorBufferBlurTexture(width());

    gBufferFBO = createGBufferFBOPointer();
    ssaoFBO = createSSAOFBOPointer();
    ssaoBlurFBO = createSSAOBlurFBOPointer();

    generateSSAOKernelAndSSAONoise();
    generateNoiseTexture();
    // --------------------------------------- //

    glSetting();
    initGeometry();
}
void GLWidget::generateSSAOKernelAndSSAONoise() {
    static QRandomGenerator *randomEngine = QRandomGenerator::global();
    static std::uniform_real_distribution<float> random(0, 1);

    for (unsigned int i = 0; i < 128; ++i)
    {
        QVector3D sample(random(*randomEngine) * 2.0 - 1.0, random(*randomEngine) * 2.0 - 1.0, random(*randomEngine));
        sample = sample.normalized();
        sample *= random(*randomEngine);
        float scale = float(i) / 128.0f;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    for (unsigned int i = 0; i < 16; i++)
    {
        QVector3D noise(random(*randomEngine) * 2.0 - 1.0, random(*randomEngine) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
}

void GLWidget::paintGL() {
    // ----- render gBuffer start ----- //
    gBufferFBO->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    SHADER(1)->bind();

    model.setToIdentity();
    model.rotate(-90,1,0,0);
    model.rotate(-135,0,0,1);

//    glActiveTexture(GL_TEXTURE0);
//    SHADER(1)->setUniformValue("colorMap", 0);
//    roomTexture->bind();

    customGeometry->drawGeometry(
            SHADER(1),
            model,
            camera->getCameraView(),
            camera->getCameraProjection());
    gBufferFBO->release();
    // ----- render gBuffer end ----- //

    if (multiSample) {
        QOpenGLFramebufferObject::bindDefault();
        getGBufferFBOAttachmentTexture();
    }

    // ----- render ssaoFBO start ----- //
    ssaoFBO->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SHADER(2)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(2)->setUniformValue("gPositionDepth", 0);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[0]);
    else
        glBindTexture(GL_TEXTURE_2D, gPositionDepth->textureId());

    glActiveTexture(GL_TEXTURE1);
    SHADER(2)->setUniformValue("gNormal", 1);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[1]);
    else
        glBindTexture(GL_TEXTURE_2D, gNormal->textureId());

    glActiveTexture(GL_TEXTURE2);
    SHADER(2)->setUniformValue("texNoise", 2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture->textureId());

    SHADER(2)->setUniformValueArray(
            "samples",
            ssaoKernel.data(),
            ssaoKernel.count());

    SHADER(2)->setUniformValue("projection", camera->getCameraProjection());

    rectGeometry->drawGeometry(SHADER(2));
    ssaoFBO->release();
    // ----- render ssaoFBO end ----- //

    // render ssaoFBOBlur start
    ssaoBlurFBO->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SHADER(3)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(3)->setUniformValue("ssaoInput", 0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer->textureId());

    rectGeometry->drawGeometry(SHADER(3));
    ssaoBlurFBO->release();
    // render ssaoFBOBlur end

    // render default frame buffer
    QOpenGLFramebufferObject::bindDefault();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SHADER(4)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(4)->setUniformValue("gPositionDepth", 0);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[0]);
    else
        glBindTexture(GL_TEXTURE_2D, gPositionDepth->textureId());

    glActiveTexture(GL_TEXTURE1);
    SHADER(4)->setUniformValue("gNormal", 1);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[1]);
    else
        glBindTexture(GL_TEXTURE_2D, gNormal->textureId());

    glActiveTexture(GL_TEXTURE2);
    SHADER(4)->setUniformValue("gAlbedo", 2);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[2]);
    else
        glBindTexture(GL_TEXTURE_2D, gAlbedo->textureId());

    glActiveTexture(GL_TEXTURE3);
    SHADER(4)->setUniformValue("ssao", 3);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur->textureId());

    glActiveTexture(GL_TEXTURE4);
    SHADER(4)->setUniformValue("mask", 4);
    if (multiSample)
        glBindTexture(GL_TEXTURE_2D, gBufferTextures[3]);
    else
        glBindTexture(GL_TEXTURE_2D, mask->textureId());

    QVector3D lightPos = QVector3D(2.0, 4.0, -2.0);
    QVector3D lightColor = QVector3D(0.5, 0.5, 0.5);
    const float constant = 1.0;
    const float linear = 0.09;
    const float quadratic = 0.032;

    QVector3D lightPosView = camera->getCameraView() * lightPos;
    SHADER(4)->setUniformValue("light.Position", lightPosView);
    SHADER(4)->setUniformValue("light.Color", lightColor);
    SHADER(4)->setUniformValue("light.Linear", linear);
    SHADER(4)->setUniformValue("light.Quadratic", quadratic);

    rectGeometry->drawGeometry(SHADER(4));

    // Debug
//    QOpenGLFramebufferObject::bindDefault();
//
//    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    SHADER(5)->bind();
//
//    glActiveTexture(GL_TEXTURE0);
//    SHADER(5)->setUniformValue("map", 0);
//    glBindTexture(GL_TEXTURE_2D, gBufferTextures[0]);
//    rectGeometry->drawGeometry(SHADER(5));
}

void GLWidget::getGBufferFBOAttachmentTexture() {
    if (gBufferTextures.size() != 4) {
        gBufferTextures.clear();
        gBufferTextures.resize(0);
    }

    QScopedPointer<QOpenGLFramebufferObject> tmpFBO;

    for (int i = 0 ; i < 4; i++) {
        tmpFBO.reset(new QOpenGLFramebufferObject(gBufferFBO->size()));
        QOpenGLFramebufferObject::blitFramebuffer(
                tmpFBO.data(),
                QRect(0,0,width(),height()),
                gBufferFBO,
                QRect(0,0,width(),height()),
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST,
                i,
                0);
        gBufferTextures[i] = tmpFBO->takeTexture(0);
    }
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    if (gBufferFBO != nullptr) {
        delete gBufferFBO;
        gBufferFBO = nullptr;
    }

    if (ssaoFBO != nullptr) {
        delete ssaoFBO;
        ssaoFBO = nullptr;
    }

    if (ssaoBlurFBO != nullptr) {
        delete ssaoBlurFBO;
        ssaoBlurFBO = nullptr;
    }

    gBufferFBO = createGBufferFBOPointer();
    ssaoFBO = createSSAOFBOPointer();
    ssaoBlurFBO = createSSAOBlurFBOPointer();

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/CustomGeometry.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/CustomGeometry.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssaoGeometry.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssaoGeometry.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssao.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssao.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();

    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssao.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssaoBlur.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
    if (!SHADER(3)->bind())
        close();

    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssao.vs.glsl"))
        close();
    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/17_ScreenSpaceAmbientOcclusion/Shaders/ssaoLighting.fs.glsl"))
        close();
    if (!SHADER(4)->link())
        close();
    if (!SHADER(4)->bind())
        close();

    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
        close();
    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/ScreenPlane.fs.glsl"))
        close();
    if (!SHADER(5)->link())
        close();
    if (!SHADER(5)->bind())
        close();
}

void GLWidget::initGeometry() {
//    customGeometry = new CustomGeometry(QString("src/resource/viking_room.obj"));
    customGeometry = new CustomGeometry(QString("src/resource/backpack.obj"));
    customGeometry->initGeometry();
    customGeometry->initAllocate();
    customGeometry->setupAttributePointer(SHADER(0));
    customGeometry->setupAttributePointer(SHADER(1));

    rectGeometry = new RectangleGeometry;
    rectGeometry->initGeometry();
    rectGeometry->setupAttributePointer(SHADER(2));
    rectGeometry->setupAttributePointer(SHADER(3));
    rectGeometry->setupAttributePointer(SHADER(4));
    rectGeometry->setupAttributePointer(SHADER(5));
}

void GLWidget::initTexture() {
    roomTexture = new QOpenGLTexture(QImage(QString("src/resource/viking_room.png")));
}

void GLWidget::generateGBufferTexture(int precision) {
    gPositionDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
    gNormal = new QOpenGLTexture(QOpenGLTexture::Target2D);
    gAlbedo = new QOpenGLTexture(QOpenGLTexture::Target2D);
    mask = new QOpenGLTexture(QOpenGLTexture::Target2D);

    gPositionDepth->create();
    gNormal->create();
    gAlbedo->create();
    mask->create();

    gPositionDepth->setFormat(QOpenGLTexture::RGBA16F);
    gNormal->setFormat(QOpenGLTexture::RGBA16F);
    gAlbedo->setFormat(QOpenGLTexture::RGBA8_UNorm);
    mask->setFormat(QOpenGLTexture::R8_UNorm);

    gPositionDepth->setSize(precision, precision, 3);
    gNormal->setSize(precision, precision, 3);
    gAlbedo->setSize(precision, precision, 3);
    mask->setSize(precision, precision, 1);

    gPositionDepth->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float16);
    gNormal->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float16);
    gAlbedo->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    mask->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

    gPositionDepth->setWrapMode(QOpenGLTexture::ClampToBorder);
    gPositionDepth->setMinificationFilter(QOpenGLTexture::Nearest);
    gPositionDepth->setMagnificationFilter(QOpenGLTexture::Nearest);

    gNormal->setMinificationFilter(QOpenGLTexture::Nearest);
    gNormal->setMagnificationFilter(QOpenGLTexture::Nearest);

    gAlbedo->setMinificationFilter(QOpenGLTexture::Nearest);
    gAlbedo->setMagnificationFilter(QOpenGLTexture::Nearest);

    mask->setMinificationFilter(QOpenGLTexture::Nearest);
    mask->setMagnificationFilter(QOpenGLTexture::Nearest);
}

void GLWidget::generateSSAOColorBufferTexture(int precision) {
    ssaoColorBuffer = new QOpenGLTexture(QOpenGLTexture::Target2D);
    ssaoColorBuffer->create();
    ssaoColorBuffer->setFormat(QOpenGLTexture::R32F);
    ssaoColorBuffer->setSize(precision, precision, 1);
    ssaoColorBuffer->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
    ssaoColorBuffer->setMinificationFilter(QOpenGLTexture::Nearest);
    ssaoColorBuffer->setMagnificationFilter(QOpenGLTexture::Nearest);
}

void GLWidget::generateSSAOColorBufferBlurTexture(int precision) {
    ssaoColorBufferBlur = new QOpenGLTexture(QOpenGLTexture::Target2D);
    ssaoColorBufferBlur->create();
    ssaoColorBufferBlur->setFormat(QOpenGLTexture::R32F);
    ssaoColorBufferBlur->setSize(precision, precision, 1);
    ssaoColorBufferBlur->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
    ssaoColorBufferBlur->setMinificationFilter(QOpenGLTexture::Nearest);
    ssaoColorBufferBlur->setMagnificationFilter(QOpenGLTexture::Nearest);
}

void GLWidget::generateNoiseTexture() {
    noiseTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    noiseTexture->create();
    noiseTexture->setFormat(QOpenGLTexture::RGB32F);
    noiseTexture->setSize(4, 4, 3);
    noiseTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::Float32);
    noiseTexture->bind();
    noiseTexture->setData(0, QOpenGLTexture::RGB, QOpenGLTexture::Float32, ssaoNoise.constData());

    noiseTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    noiseTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    noiseTexture->setWrapMode(QOpenGLTexture::Repeat);
}

QOpenGLFramebufferObject *GLWidget::createGBufferFBOPointer() {
    QOpenGLFramebufferObject *bufferObject;

    if (multiSample) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::Depth);
        format.setTextureTarget(QOpenGLTexture::Target2DMultisample);
        format.setSamples(16);

        QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
        bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

        bufferObject->bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, gPositionDepth->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize, QOpenGLTexture::RGBA16F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, gNormal->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize, QOpenGLTexture::RGBA8_UNorm);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, gAlbedo->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize, QOpenGLTexture::R8_UNorm);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D_MULTISAMPLE, mask->textureId(), 0);

        unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);
    }
    else {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::Depth);
        format.setTextureTarget(QOpenGLTexture::Target2D);

        QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
        bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

        bufferObject->bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionDepth->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo->textureId(), 0);

        bufferObject->addColorAttachment(frameBufferSize);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mask->textureId(), 0);

        unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);
    }

    qDebug() << "GBuffer FBO attachment count:" << bufferObject->sizes().size();
    
    return bufferObject;
}

QOpenGLFramebufferObject *GLWidget::createSSAOFBOPointer() {
    QOpenGLFramebufferObjectFormat format;
    format.setTextureTarget(QOpenGLTexture::Target2D);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    auto bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

    //
    bufferObject->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer->textureId(), 0);

    return bufferObject;
}

QOpenGLFramebufferObject *GLWidget::createSSAOBlurFBOPointer() {
    QOpenGLFramebufferObjectFormat format;
    format.setTextureTarget(QOpenGLTexture::Target2D);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    auto bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

    //
    bufferObject->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur->textureId(), 0);

    return bufferObject;
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
    delete roomTexture;
    delete rectGeometry;

    delete gBufferFBO;
    delete ssaoFBO;
    delete ssaoBlurFBO;
    delete gPositionDepth;
    delete gNormal;
    delete gAlbedo;
    delete mask;
    delete ssaoColorBuffer;
    delete ssaoColorBufferBlur;
    delete noiseTexture;

    camera = nullptr;
    customGeometry = nullptr;
    rectGeometry = nullptr;
    roomTexture = nullptr;

    gBufferFBO = nullptr;
    ssaoFBO = nullptr;
    ssaoBlurFBO = nullptr;

    gPositionDepth = nullptr;
    gNormal = nullptr;
    gAlbedo = nullptr;
    mask = nullptr;
    ssaoColorBuffer = nullptr;
    ssaoColorBufferBlur = nullptr;
    noiseTexture = nullptr;

    //gBufferTextures.clear();  // got debug error

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