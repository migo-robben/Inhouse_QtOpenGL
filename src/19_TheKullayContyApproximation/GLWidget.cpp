#include "GLWidget.h"
#include <QKeyEvent>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          hdrTexture(nullptr),
          envCubemap(nullptr),
          irradianceMap(nullptr),
          prefilterMap(nullptr),
          brdfLUTTexture(nullptr),
          captureFBO(nullptr),
          cubeGeometry(nullptr),
          skybox_geometry(nullptr),
          sphereGeometry(nullptr),
          QuadGeometry(nullptr),
          camera(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<8; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 20);
    camera = new Camera(cameraPos);
}

GLWidget::~GLWidget() {
    cleanup();
}

QSize GLWidget::minimumSizeHint() const {
    return {50, 50};
}

QSize GLWidget::sizeHint() const {
    return {1080, 540};
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

    captureFBO = createFBOPointer(0);
    renderEnvCubeMap(512);
    renderIrradianceMap(128);
    renderPrefilterMap(128);
    renderBRDFMap(512);
}

void GLWidget::paintGL() {
    QOpenGLFramebufferObject::bindDefault();

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    renderSphere(SHADER(3), 5.0f, true, computePointLight); // Multiple Scattering
    renderSphere(SHADER(4), 2.5f, true, computePointLight); // Single Scattering

    renderSphere(SHADER(3), -2.5f, false, computePointLight); // Multiple Scattering
    renderSphere(SHADER(4), -5.0f, false, computePointLight); // Single Scattering

    // ----- Render background ----- //
    SHADER(6)->bind();
    glActiveTexture(GL_TEXTURE0);
    SHADER(6)->setUniformValue("map", 0);
    irradianceMap->bind();
    glDepthFunc(GL_LEQUAL);
    skybox_geometry->drawGeometry(
            SHADER(6),
            camera->getCameraView(),
            camera->getCameraProjection());
    irradianceMap->release();
    glDepthFunc(GL_LESS);

    // debug render 2D LUT map
//    SHADER(7)->bind();
//    QuadGeometry->drawGeometry(
//            SHADER(7),
//            brdfLUTTexture);
}

void GLWidget::renderSphere(QOpenGLShaderProgram *shader, float YOffset, bool is_metal, bool isComputePointLight) {
    // Lights
    QVector<QVector3D> lightPositions{
            QVector3D(-10.0f,  10.0f, 10.0f),
            QVector3D( 10.0f,  10.0f, 10.0f),
            QVector3D(-10.0f, -10.0f, 10.0f),
            QVector3D( 10.0f, -10.0f, 10.0f),
    };
    QVector<QVector3D> lightColors{
            QVector3D(300.0f, 300.0f, 300.0f),
            QVector3D(300.0f, 300.0f, 300.0f),
            QVector3D(300.0f, 300.0f, 300.0f),
            QVector3D(300.0f, 300.0f, 300.0f)
    };

    shader->bind();

    shader->setUniformValue("albedo", QVector3D(1.0f, 1.0f, 1.0f));
    shader->setUniformValue("ao", 1.0f);
    shader->setUniformValue("camPos", camera->getCameraPosition());
    shader->setUniformValue("isMetal", is_metal);
    shader->setUniformValue("computePointLight", isComputePointLight);

    shader->setUniformValueArray("lightPositions", lightPositions.data(), lightPositions.size());
    shader->setUniformValueArray("lightColors", lightColors.data(), lightColors.size());

    glActiveTexture(GL_TEXTURE0);
    shader->setUniformValue("irradianceMap", 0);
    irradianceMap->bind();

    glActiveTexture(GL_TEXTURE1);
    shader->setUniformValue("prefilterMap", 1);
    prefilterMap->bind();

    glActiveTexture(GL_TEXTURE2);
    shader->setUniformValue("brdfLUT", 2);
    brdfLUTTexture->bind();

    int nrColumns = 10;
    float spacing = 3.0;
    for (int col = 0; col < nrColumns; col++) {
        shader->setUniformValue("metallic", is_metal ? 1.0f : 0.0f);

        shader->setUniformValue("roughness", qBound(0.0f,(float)col / (float)(nrColumns - 1),1.0f));
        model.setToIdentity();
        model.translate(((float)col - ((float)nrColumns / 2.0f)) * spacing + 1.2f,
                        YOffset,
                        0.0f);

        sphereGeometry->drawGeometry(
                shader,
                model,
                camera->getCameraView(),
                camera->getCameraProjection());
    }

    shader->release();
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    if (captureFBO != nullptr) {
        delete captureFBO;
        captureFBO = nullptr;
    }
    captureFBO = createFBOPointer(0);

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    // for cube box
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/equirectangular_to_map.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    // irradiance
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/irradiance_convolution.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    // prefilter
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/prefilter.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();

    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/pbr.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/multipleScatterPBR.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
    if (!SHADER(3)->bind())
        close();

    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/pbr.vs.glsl"))
        close();
    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/singleScatterPBR.fs.glsl"))
        close();
    if (!SHADER(4)->link())
        close();
    if (!SHADER(4)->bind())
        close();

    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/brdf.vs.glsl"))
        close();
    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/brdf.fs.glsl"))
        close();
    if (!SHADER(5)->link())
        close();
    if (!SHADER(5)->bind())
        close();

    if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/19_TheKullayContyApproximation/Shaders/background.vs.glsl"))
        close();
    if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/19_TheKullayContyApproximation/Shaders/background.fs.glsl"))
        close();
    if (!SHADER(6)->link())
        close();
    if (!SHADER(6)->bind())
        close();

    // Debug shader
    if (!SHADER(7)->addShaderFromSourceFile(QOpenGLShader::Vertex, "F:/CLionProjects/QtReference/src/17_qopengl_mess/Shaders/ScreenQuad.vs.glsl"))
        close();
    if (!SHADER(7)->addShaderFromSourceFile(QOpenGLShader::Fragment, "F:/CLionProjects/QtReference/src/17_qopengl_mess/Shaders/ScreenQuad.fs.glsl"))
        close();
    if (!SHADER(7)->link())
        close();
    if (!SHADER(7)->bind())
        close();
}

void GLWidget::initGeometry() {
    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(0));

    sphereGeometry = new SphereGeometry;
    sphereGeometry->initGeometry();
    sphereGeometry->setupAttributePointer(SHADER(3));

    QuadGeometry = new RectangleGeometry;
    QuadGeometry->initGeometry();
    QuadGeometry->setupAttributePointer(SHADER(5));

    skybox_geometry = new SkyboxGeometry;
    skybox_geometry->initGeometry();
    skybox_geometry->setupAttributePointer(SHADER(6));
}

void GLWidget::initTexture() {
    loadHDRTextrue();
    generateEnvCubeMap(512);
    generateIrradianceMap(128);
    generatePrefilterMap(128);
    generateBRDFMap(512);
}

void GLWidget::loadHDRTextrue() {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    float *image = nullptr;
    if (furnaceTest)
        image = stbi_loadf("src/texture/HDR/Uniform.jpg", &width, &height, &channels, 0);
    else
        image = stbi_loadf("src/texture/HDR/newport_loft.hdr", &width, &height, &channels, 0);

    hdrTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    hdrTexture->create();
    hdrTexture->setSize(width, height, 1);
    hdrTexture->setFormat(QOpenGLTexture::RGB32F);
    hdrTexture->allocateStorage();

    hdrTexture->setData(0,0,QOpenGLTexture::RGB,QOpenGLTexture::Float32, image,Q_NULLPTR);
    hdrTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    hdrTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    hdrTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

    stbi_image_free(image);
}

void GLWidget::generateEnvCubeMap(int precision) {
    envCubemap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    envCubemap->create();
    envCubemap->setSize(precision, precision, 3);
    envCubemap->setFormat(QOpenGLTexture::RGB32F);
    envCubemap->setMipLevels(envCubemap->maximumMipLevels());
    envCubemap->allocateStorage();
    envCubemap->bind();

    envCubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    envCubemap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    envCubemap->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLWidget::generateIrradianceMap(int precision) {
    irradianceMap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    irradianceMap->create();
    irradianceMap->setSize(precision, precision, 3);
    irradianceMap->setFormat(QOpenGLTexture::RGB32F);
    irradianceMap->allocateStorage();
    irradianceMap->bind();

    irradianceMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    irradianceMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    irradianceMap->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLWidget::generatePrefilterMap(int precision) {
    prefilterMap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    prefilterMap->create();
    prefilterMap->setSize(precision, precision, 3);
    prefilterMap->setFormat(QOpenGLTexture::RGB32F);
    prefilterMap->setMipLevels(prefilterMap->maximumMipLevels());
    prefilterMap->allocateStorage();
    prefilterMap->bind();

    prefilterMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    prefilterMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    prefilterMap->setMagnificationFilter(QOpenGLTexture::Linear);
    prefilterMap->generateMipMaps();
}

void GLWidget::generateBRDFMap(int precision) {
    brdfLUTTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    brdfLUTTexture->create();
    brdfLUTTexture->setSize(precision, precision, 3);
    brdfLUTTexture->setFormat(QOpenGLTexture::RGB32F);prefilterMap->allocateStorage();
    brdfLUTTexture->allocateStorage();
    brdfLUTTexture->bind();

    brdfLUTTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    brdfLUTTexture->setMinificationFilter(QOpenGLTexture::Linear);
    brdfLUTTexture->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLWidget::renderEnvCubeMap(int precision) {
    // rendering
    SHADER(0)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(0)->setUniformValue("equirectangularMap", 0);
    hdrTexture->bind();
    SHADER(0)->setUniformValue("projection", captureProjection);

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    for(int i=0; i<captureViews.count(); ++i) {
        SHADER(0)->setUniformValue("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap->textureId(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cubeGeometry->drawGeometry(
                SHADER(0), hdrTexture);
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    envCubemap->bind();
    envCubemap->generateMipMaps();
    envCubemap->release();
}

void GLWidget::renderIrradianceMap(int precision) {
    // rendering
    SHADER(1)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(1)->setUniformValue("environmentMap", 0);
    envCubemap->bind();
    SHADER(1)->setUniformValue("projection", captureProjection);

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    for(int i=0; i<captureViews.count(); ++i) {
        SHADER(1)->setUniformValue("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->textureId(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cubeGeometry->drawGeometry(
                SHADER(1), envCubemap);
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void GLWidget::renderPrefilterMap(int precision) {
    SHADER(2)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(2)->setUniformValue("environmentMap", 0);
    envCubemap->bind();
    SHADER(2)->setUniformValue("projection", captureProjection);

    captureFBO->bind();
    unsigned int maxMipLevels = prefilterMap->maximumMipLevels();
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth  = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);

        glViewport(0, 0, mipWidth * devicePixelRatio(), mipHeight * devicePixelRatio());
        float roughness = (float)mip / (float)(maxMipLevels - 1);
        SHADER(2)->setUniformValue("roughness", roughness);
        for (int i=0; i<captureViews.count(); ++i) {
            SHADER(2)->setUniformValue("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->textureId(), mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cubeGeometry->drawGeometry(
                    SHADER(2), envCubemap);
        }
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void GLWidget::renderBRDFMap(int precision) {
    SHADER(5)->bind();

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture->textureId(), 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QuadGeometry->drawGeometry(
            SHADER(5));

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void GLWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Setting
    captureProjection.setToIdentity();
    captureProjection.perspective(90.0f, 1.0f, 0.1f, 10.0f);

    QMatrix4x4 lookAt_1;
    lookAt_1.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f));
    QMatrix4x4 lookAt_2;
    lookAt_2.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f));
    QMatrix4x4 lookAt_3;
    lookAt_3.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f));
    QMatrix4x4 lookAt_4;
    lookAt_4.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f));
    QMatrix4x4 lookAt_5;
    lookAt_5.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, -1.0f, 0.0f));
    QMatrix4x4 lookAt_6;
    lookAt_6.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f), QVector3D(0.0f, -1.0f, 0.0f));
    captureViews << lookAt_1 << lookAt_2 << lookAt_3 << lookAt_4 << lookAt_5 << lookAt_6;
}

QOpenGLFramebufferObject* GLWidget::createFBOPointer(int sampleCount) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(QOpenGLTexture::RGB32F);
    format.setSamples(sampleCount);
    format.setMipmap(true);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    return new QOpenGLFramebufferObject(frameBufferSize, format);
}

void GLWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete hdrTexture;
    delete envCubemap;
    delete irradianceMap;
    delete prefilterMap;
    delete brdfLUTTexture;
    delete captureFBO;
    delete cubeGeometry;
    delete skybox_geometry;
    delete sphereGeometry;
    delete QuadGeometry;

    camera = nullptr;
    hdrTexture = nullptr;
    envCubemap = nullptr;
    irradianceMap = nullptr;
    prefilterMap = nullptr;
    brdfLUTTexture = nullptr;
    captureFBO = nullptr;
    cubeGeometry = nullptr;
    skybox_geometry = nullptr;
    sphereGeometry = nullptr;
    QuadGeometry = nullptr;

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