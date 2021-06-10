#include "GLWidget.h"
#include <QKeyEvent>
#include <QRandomGenerator>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          hdrTexture(nullptr),
          envCubeMap(nullptr),
          irradianceMap(nullptr),
          prefilterMap(nullptr),
          BRDFMap(nullptr),
          captureFBO(nullptr),
          gBuffer(nullptr),
          cubeGeometry(nullptr),
          skyboxGeometry(nullptr),
          rectGeometry(nullptr),
          camera(nullptr),
          BackDrop(nullptr),
          ShaderBall(nullptr),
          noiseTexture(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<12; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 55);
    camera = new Camera(cameraPos);
    camera->setCameraNearClipPlane(0.01);
    camera->setCameraFarClipPlane(1000.0);

    // ----- Init PreFrame ----- //
    preViewMatrix = camera->getCameraView();
}

GLWidget::~GLWidget() {
    cleanup();
}

QSize GLWidget::minimumSizeHint() const {
    return {50, 50};
}

QSize GLWidget::sizeHint() const {
    return {512, 512};
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
    gBuffer    = createGBufferFBOPointer();
    pbrBuffer  = createSimpleFBOPointer();
    createSSRFBOPointer();
    compositeBuffer = createSimpleFBOPointer();
    createTAAFBOPointer();

    renderEnvCubeMap(512);
    renderIrradianceMap(128);
    renderPrefilterMap(128);
    renderBRDFMap(512);

    myTimer.start();
}

void GLWidget::paintGL() {
    bool renderGBuffer = true;
    bool renderPBRBuffer = true;
    bool renderSSRBuffer = true;
    bool renderCompositeProcess = true;
    bool renderTAABuffer = false;

    bool renderDebugScreenQuad = true;
    bool renderDebugBackground = false;
    bool renderBackDrop = true;

    float spacing = 8.0;
    int nrColumns = 4;
    QVector3D BackDropTr = QVector3D(0.0f, -3.5f, 0.0f);

    static QRandomGenerator *randomEngine = QRandomGenerator::global();
    static std::uniform_real_distribution<float> random(0, 1);
    QVector2D randomSeed = QVector2D(random(*randomEngine), random(*randomEngine));

    // Render gBuffer
    if (renderGBuffer) {
        gBuffer->bind();

        unsigned int maxMipLevels = gAlbedoSpec->maximumMipLevels();
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth = width() * std::pow(0.5, mip);
            unsigned int mipHeight = height() * std::pow(0.5, mip);

            glViewport(0, 0, mipWidth * devicePixelRatio(), mipHeight * devicePixelRatio());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gAlbedoSpec->textureId(), mip);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gExpensiveNormal->textureId(), mip);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDepth->textureId(), mip);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gExtraComponents->textureId(), mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glEnable(GL_DEPTH_TEST);

            SHADER(7)->bind();
            {
                for (int col = 0; col < nrColumns; col++) {
                    model.setToIdentity();
                    model.translate((col - (nrColumns / 2.0) + 0.5f) * spacing,
                                    0.005f,
                                    0.0f);
                    model.scale(3.5f);

                    glActiveTexture(GL_TEXTURE0);
                    SHADER(7)->setUniformValue("albedoMap", 0);
                    albedo_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE1);
                    SHADER(7)->setUniformValue("roughnessMap", 1);
                    roughness_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE2);
                    SHADER(7)->setUniformValue("normalMap", 2);
                    normal_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE3);
                    SHADER(7)->setUniformValue("metallicMap", 3);
                    metallic_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE4);
                    SHADER(7)->setUniformValue("skybox", 4);
                    envCubeMap->bind();

                    SHADER(7)->setUniformValue("PrevModel", model);
                    SHADER(7)->setUniformValue("PrevView", preViewMatrix);
                    SHADER(7)->setUniformValue("viewPos", camera->getCameraPosition());

                    ShaderBall->drawGeometry(
                            SHADER(7),
                            model,
                            camera->getCameraView(),
                            camera->getCameraProjection());
                }
                if (renderBackDrop) {
                    model.setToIdentity();
                    model.translate(BackDropTr);
                    model.scale(30.0f);

                    BackDrop->drawGeometry(
                            SHADER(7),
                            model,
                            camera->getCameraView(),
                            camera->getCameraProjection());
                }
            }
        }

        gBuffer->release();
    }

    // Render PBR
    if (renderPBRBuffer) {
        pbrBuffer->bind();

        unsigned int maxMipLevels = bprColorTexture->maximumMipLevels();
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth = width() * std::pow(0.5, mip);
            unsigned int mipHeight = height() * std::pow(0.5, mip);

            glViewport(0, 0, mipWidth * devicePixelRatio(), mipHeight * devicePixelRatio());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bprColorTexture->textureId(), mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glEnable(GL_DEPTH_TEST);

            SHADER(6)->bind();
            {
                // Render balls
                QVector<QVector3D> lightPositions{
                        QVector3D(-10.0f, 10.0f, 10.0f),
                        QVector3D(10.0f, 10.0f, 10.0f),
                        QVector3D(-10.0f, -10.0f, 10.0f),
                        QVector3D(10.0f, -10.0f, 10.0f),
                };
                QVector<QVector3D> lightColors{
                        QVector3D(300.0f, 300.0f, 300.0f),
                        QVector3D(300.0f, 300.0f, 300.0f),
                        QVector3D(300.0f, 300.0f, 300.0f),
                        QVector3D(300.0f, 300.0f, 300.0f)
                };

                for (int col = 0; col < nrColumns; col++) {
                    model.setToIdentity();
                    model.translate((col - (nrColumns / 2.0) + 0.5f) * spacing,
                                    0.005f,
                                    0.0f);
                    model.scale(3.5f);

                    SHADER(6)->setUniformValue("camPos", camera->getCameraPosition());

                    glActiveTexture(GL_TEXTURE0);
                    SHADER(6)->setUniformValue("irradianceMap", 0);
                    irradianceMap->bind();

                    glActiveTexture(GL_TEXTURE1);
                    SHADER(6)->setUniformValue("prefilterMap", 1);
                    prefilterMap->bind();

                    glActiveTexture(GL_TEXTURE2);
                    SHADER(6)->setUniformValue("brdfLUT", 2);
                    BRDFMap->bind();

                    glActiveTexture(GL_TEXTURE3);
                    SHADER(6)->setUniformValue("albedoMap", 3);
                    albedo_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE4);
                    SHADER(6)->setUniformValue("metallicMap", 4);
                    metallic_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE5);
                    SHADER(6)->setUniformValue("roughnessMap", 5);
                    roughness_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE6);
                    SHADER(6)->setUniformValue("aoMap", 6);
                    ao_textures[col]->bind();

                    glActiveTexture(GL_TEXTURE7);
                    SHADER(6)->setUniformValue("normalMap", 7);
                    normal_textures[col]->bind();

                    SHADER(6)->setUniformValueArray("lightPositions", lightPositions.data(), lightPositions.size());
                    SHADER(6)->setUniformValueArray("lightColors", lightColors.data(), lightColors.size());

                    ShaderBall->drawGeometry(
                            SHADER(6),
                            model,
                            camera->getCameraView(),
                            camera->getCameraProjection());
                }
                if (renderBackDrop) {
                    model.setToIdentity();
                    model.translate(BackDropTr);
                    model.scale(30.0f);

                    BackDrop->drawGeometry(
                            SHADER(6),
                            model,
                            camera->getCameraView(),
                            camera->getCameraProjection());
                }
            }
        }

        pbrBuffer->release();
    }

    // Render SSR
    if (renderSSRBuffer) {
        ssrBuffer[CurrentSSR]->bind();

        unsigned int maxMipLevels = ssrTexture[CurrentSSR]->maximumMipLevels();
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth = width() * std::pow(0.5, mip);
            unsigned int mipHeight = height() * std::pow(0.5, mip);

            glViewport(0, 0, mipWidth * devicePixelRatio(), mipHeight * devicePixelRatio());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssrTexture[CurrentSSR]->textureId(), mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glDisable(GL_BLEND);

            SHADER(8)->bind();

            glActiveTexture(GL_TEXTURE0);
            SHADER(8)->setUniformValue("gFinalImage", 0);
            bprColorTexture->bind();

            glActiveTexture(GL_TEXTURE1);
            SHADER(8)->setUniformValue("gNormal", 1);
            gExpensiveNormal->bind();

            glActiveTexture(GL_TEXTURE2);
            SHADER(8)->setUniformValue("gExtraComponents", 2);
            gExtraComponents->bind();

            glActiveTexture(GL_TEXTURE3);
            SHADER(8)->setUniformValue("gDepth", 3);
            gDepth->bind();

            glActiveTexture(GL_TEXTURE4);
            SHADER(8)->setUniformValue("BRDF", 4);
            BRDFMap->bind();

            glActiveTexture(GL_TEXTURE4);
            SHADER(8)->setUniformValue("noiseTexture", 4);
            noiseTexture->bind();

            glActiveTexture(GL_TEXTURE5);
            SHADER(8)->setUniformValue("PreviousReflection", 5);
            ssrTexture[!CurrentSSR]->bind();

            glActiveTexture(GL_TEXTURE6);
            SHADER(8)->setUniformValue("gAlbedo", 6);
            gAlbedoSpec->bind();

            SHADER(8)->setUniformValue("view", camera->getCameraView());
            SHADER(8)->setUniformValue("projection", camera->getCameraProjection());
            SHADER(8)->setUniformValue("camPos", camera->getCameraPosition());
            SHADER(8)->setUniformValue("Resolution", QVector2D(width(), height()));

            QVector2D LinMAD(
                    (camera->getCameraNearClipPlane() - camera->getCameraFarClipPlane()) / ( 2.0 * camera->getCameraNearClipPlane() * camera->getCameraFarClipPlane()),
                    (camera->getCameraNearClipPlane() + camera->getCameraFarClipPlane()) / ( 2.0 * camera->getCameraNearClipPlane() * camera->getCameraFarClipPlane()));
            SHADER(8)->setUniformValue("LinMAD", LinMAD);

            float time = (float)myTimer.elapsed() * 1.0 / 1000.0;
            SHADER(8)->setUniformValue("Time", time);

            SHADER(8)->setUniformValue("ID", TotalFrames);
            SHADER(8)->setUniformValue("isMoving", isMoving);

            rectGeometry->drawGeometry(SHADER(8));
        }

        ssrBuffer[CurrentSSR]->release();
        CurrentSSR = !CurrentSSR;
    }

    // Render Composite
    if (renderCompositeProcess)
    {
        compositeBuffer->bind();

        if (isMoving)
            TotalFrames = 0;
        else
            TotalFrames++;

        SHADER(9)->bind();
        glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTexture->textureId(), 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        glActiveTexture(GL_TEXTURE0);
        SHADER(9)->setUniformValue("gReflectionSampler", 0);
        if (TotalFrames >=250 )
            glBindTexture(GL_TEXTURE_2D, ssrTexture[!CurrentSSR]->textureId());
        else
            glBindTexture(GL_TEXTURE_2D, ssrTexture[CurrentSSR]->textureId());

        glActiveTexture(GL_TEXTURE1);
        SHADER(9)->setUniformValue("gColorSampler", 1);
        glBindTexture(GL_TEXTURE_2D, bprColorTexture->textureId());

        SHADER(9)->setUniformValue("TotalFrames", TotalFrames);
        SHADER(9)->setUniformValue("isMoving", isMoving);
        
        rectGeometry->drawGeometry(SHADER(9));

        compositeBuffer->release();
    }

    // Render TAA
    if (renderTAABuffer) {
        TAABuffer[TAACurrentBuffer]->bind();

        glDisable(GL_BLEND);

        glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TAATexture[TAACurrentBuffer]->textureId(), 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        SHADER(10)->bind();

        glActiveTexture(GL_TEXTURE0);
        SHADER(10)->setUniformValue("sCurrentFrame", 0);
        glBindTexture(GL_TEXTURE_2D, compositeTexture->textureId());

        glActiveTexture(GL_TEXTURE1);
        SHADER(10)->setUniformValue("sLastFrame", 1);
        glBindTexture(GL_TEXTURE_2D, TAATexture[TAACurrentBuffer]->textureId());

        glActiveTexture(GL_TEXTURE2);
        SHADER(10)->setUniformValue("sVelocityBuffer", 2);
        glBindTexture(GL_TEXTURE_2D, gExtraComponents->textureId());

        glActiveTexture(GL_TEXTURE3);
        SHADER(10)->setUniformValue("sDepthBuffer", 3);
        glBindTexture(GL_TEXTURE_2D, gDepth->textureId());

        SHADER(10)->setUniformValue("iResolution", QVector2D(width(), height()));

        rectGeometry->drawGeometry(SHADER(10));

        TAABuffer[TAACurrentBuffer]->release();
        TAACurrentBuffer = !TAACurrentBuffer;
    }

    // Debug Render Quad
    if (renderDebugScreenQuad) {
        QOpenGLFramebufferObject::bindDefault();
        glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

        SHADER(11)->bind();
        glActiveTexture(GL_TEXTURE0);
        SHADER(11)->setUniformValue("colorBuffer", 0);
        if (renderTAABuffer)
            glBindTexture(GL_TEXTURE_2D, TAATexture[TAACurrentBuffer]->textureId());
        else
            glBindTexture(GL_TEXTURE_2D, compositeTexture->textureId());

        rectGeometry->drawGeometry(SHADER(11));
    }

    // Debug Render Background
    if (renderDebugBackground) {
        QOpenGLFramebufferObject::bindDefault();
        glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

        SHADER(0)->bind();
        glActiveTexture(GL_TEXTURE0);
        SHADER(0)->setUniformValue("map", 0);
        irradianceMap->bind();
        glDepthFunc(GL_LEQUAL);

        skyboxGeometry->drawGeometry(
                SHADER(0),
                camera->getCameraView(),
                camera->getCameraProjection());
        glDepthFunc(GL_LESS);
    }

    this->updatePreStatues();
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    // Simple Shader
    {
        // skybox background
        if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/background.vs.glsl"))
            close();
        if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/background.fs.glsl"))
            close();
        if (!SHADER(0)->link())
            close();
        if (!SHADER(0)->bind())
            close();

        // Quad screen
        if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
            close();
        if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/ScreenPlane.fs.glsl"))
            close();
        if (!SHADER(1)->link())
            close();
        if (!SHADER(1)->bind())
            close();
    }

    // PBR Shader
    {
        // use for render env cube map
        if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/15_IBL_Specular_texture/shaders/cubemap.vs.glsl"))
            close();
        if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/15_IBL_Specular_texture/shaders/equirectangular_to_map.fs.glsl"))
            close();
        if (!SHADER(2)->link())
            close();
        if (!SHADER(2)->bind())
            close();

        // irradiance map
        if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/15_IBL_Specular_texture/shaders/cubemap.vs.glsl"))
            close();
        if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/15_IBL_Specular_texture/shaders/irradiance_convolution.fs.glsl"))
            close();
        if (!SHADER(3)->link())
            close();
        if (!SHADER(3)->bind())
            close();

        // prefilter map
        if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/15_IBL_Specular_texture/shaders/cubemap.vs.glsl"))
            close();
        if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/15_IBL_Specular_texture/shaders/prefilter.fs.glsl"))
            close();
        if (!SHADER(4)->link())
            close();
        if (!SHADER(4)->bind())
            close();

        // BRDF map
        if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/15_IBL_Specular_texture/shaders/brdf.vs.glsl"))
            close();
        if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/15_IBL_Specular_texture/shaders/brdf.fs.glsl"))
            close();
        if (!SHADER(5)->link())
            close();
        if (!SHADER(5)->bind())
            close();

        if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/15_IBL_Specular_texture/shaders/pbr.vs.glsl"))
            close();
        if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/15_IBL_Specular_texture/shaders/pbr.fs.glsl"))
            close();
        if (!SHADER(6)->link())
            close();
        if (!SHADER(6)->bind())
            close();
    }

    // SSR Shader
    {
        if (!SHADER(7)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/18_ScreenSpaceReflection/Shaders/Version3/gBuffer.vs.glsl"))
            close();
        if (!SHADER(7)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/18_ScreenSpaceReflection/Shaders/Version3/gBuffer.fs.glsl"))
            close();
        if (!SHADER(7)->link())
            close();
        if (!SHADER(7)->bind())
            close();

        if (!SHADER(8)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/18_ScreenSpaceReflection/Shaders/Version3/SSR.vs.glsl"))
            close();
        if (!SHADER(8)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/18_ScreenSpaceReflection/Shaders/Version3/SSR.fs.glsl"))
            close();
        if (!SHADER(8)->link())
            close();
        if (!SHADER(8)->bind())
            close();

        if (!SHADER(9)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
            close();
        if (!SHADER(9)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/18_ScreenSpaceReflection/Shaders/Version3/composite.fs.glsl"))
            close();
        if (!SHADER(9)->link())
            close();
        if (!SHADER(9)->bind())
            close();

        if (!SHADER(10)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
            close();
        if (!SHADER(10)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/18_ScreenSpaceReflection/Shaders/Version3/TAA.fs.glsl"))
            close();
        if (!SHADER(10)->link())
            close();
        if (!SHADER(10)->bind())
            close();
    }

    if (!SHADER(11)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/18_ScreenSpaceReflection/Shaders/Version3/debugScreen.vs.glsl"))
        close();
    if (!SHADER(11)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/18_ScreenSpaceReflection/Shaders/Version3/debugScreen.fs.glsl"))
        close();
    if (!SHADER(11)->link())
        close();
    if (!SHADER(11)->bind())
        close();
}

void GLWidget::initGeometry() {
    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(2));

    rectGeometry = new RectangleGeometry;
    rectGeometry->initGeometry();
    rectGeometry->setupAttributePointer(SHADER(5));
    rectGeometry->setupAttributePointer(SHADER(1));
    rectGeometry->setupAttributePointer(SHADER(8));
    rectGeometry->setupAttributePointer(SHADER(9));
    rectGeometry->setupAttributePointer(SHADER(11));

    skyboxGeometry = new SkyboxGeometry;
    skyboxGeometry->initGeometry();
    skyboxGeometry->setupAttributePointer(SHADER(0));

    BackDrop = new CustomGeometry(QString("src/18_ScreenSpaceReflection/Models/BackDrop.obj"));
    BackDrop->initGeometry();
    BackDrop->setupAttributePointer(SHADER(6));
    BackDrop->setupAttributePointer(SHADER(7));

    ShaderBall = new CustomGeometry(QString("src/18_ScreenSpaceReflection/Models/ShaderBall.obj"));
    ShaderBall->initGeometry();
    ShaderBall->setupAttributePointer(SHADER(6));
    ShaderBall->setupAttributePointer(SHADER(7));
}

void GLWidget::initTexture() {

    loadHDRTexture();
    loadMaterialTextures();

    generateEnvCubeMap(512);
    generateIrradianceMap(128);
    generatePrefilterMap(128);
    generateBRDFMap(512);

    // SSR
    generateGBufferTexture(512);
    generatePBRColorBufferTexture(512);
    generateSSRBufferTexture(512);
    generateTAABufferTexture(512);
    generateCompositeBufferTexture(512);
    
    noiseTexture = new QOpenGLTexture(QImage(QString("src/texture/LDR_RGBA_0.png")).convertToFormat(QImage::Format_RGBA8888));
}

void GLWidget::loadMaterialTextures() {
    for (const auto& alebdoFilePath : albedoTextureFilePath) {
        auto* alebdoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        auto imageData = QImage(alebdoFilePath).convertToFormat(QImage::Format_RGBA8888);

        alebdoTexture->create();
        alebdoTexture->setSize(imageData.width(), imageData.height(), imageData.depth());
        alebdoTexture->setFormat(QOpenGLTexture::SRGB8); // gamma correct
        alebdoTexture->setMipLevels(alebdoTexture->maximumMipLevels());
        alebdoTexture->allocateStorage();
        alebdoTexture->bind();

        alebdoTexture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageData.constBits());
        alebdoTexture->setWrapMode(QOpenGLTexture::Repeat);
        alebdoTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        alebdoTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        alebdoTexture->generateMipMaps();

        albedo_textures << alebdoTexture;
    }

    for (const auto& metalicFilePath : metalTextureFilePath) {
        metallic_textures << new QOpenGLTexture(QImage(metalicFilePath));
    }

    for (const auto& roughnessFilePath : roughnessTextureFilePath) {
        roughness_textures << new QOpenGLTexture(QImage(roughnessFilePath));
    }

    for (const auto& aoFilePath : aoTextureFilePath) {
        ao_textures << new QOpenGLTexture(QImage(aoFilePath));
    }

    for (const auto& normalFilePath : normalTextureFilePath) {
        normal_textures << new QOpenGLTexture(QImage(normalFilePath));
    }
}

void GLWidget::loadHDRTexture() {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    float *image = stbi_loadf("src/texture/HDR/newport_loft.hdr", &width, &height, &channels, 0);

    hdrTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    hdrTexture->create();
    hdrTexture->setSize(width, height, channels);
    hdrTexture->setFormat(QOpenGLTexture::RGB32F);
    hdrTexture->allocateStorage();

    hdrTexture->setData(0,0,QOpenGLTexture::RGB,QOpenGLTexture::Float32, image,Q_NULLPTR);
    hdrTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    hdrTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    hdrTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

    stbi_image_free(image);
}

void GLWidget::generateEnvCubeMap(int precision) {
    envCubeMap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    envCubeMap->create();
    envCubeMap->setSize(precision, precision, 1);
    envCubeMap->setFormat(QOpenGLTexture::RGB32F);
    envCubeMap->setMipLevels(envCubeMap->maximumMipLevels());
    envCubeMap->allocateStorage();
    envCubeMap->bind();

    envCubeMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    envCubeMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    envCubeMap->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLWidget::generateIrradianceMap(int precision) {
    irradianceMap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    irradianceMap->create();
    irradianceMap->setSize(precision, precision, 1);
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
    prefilterMap->setSize(precision, precision, 1);
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
    BRDFMap = new QOpenGLTexture(QOpenGLTexture::Target2D);
    BRDFMap->create();
    BRDFMap->setSize(precision, precision, 1);
    BRDFMap->setFormat(QOpenGLTexture::RGB32F);prefilterMap->allocateStorage();
    BRDFMap->allocateStorage();
    BRDFMap->bind();

    BRDFMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    BRDFMap->setMinificationFilter(QOpenGLTexture::Linear);
    BRDFMap->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLWidget::generateGBufferTexture(int precision) {
    gBufferTextures.resize(4);

    for (auto & gBufferTexture : gBufferTextures) {
        gBufferTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        gBufferTexture->create();
        gBufferTexture->setFormat(QOpenGLTexture::RGBA32F);
        gBufferTexture->setSize(precision, precision, 1);
        gBufferTexture->setMipLevels(gBufferTexture->maximumMipLevels());
        gBufferTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
        gBufferTexture->setWrapMode(QOpenGLTexture::Repeat);
        gBufferTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        gBufferTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
        gBufferTexture->generateMipMaps();
    }
    gAlbedoSpec = gBufferTextures[0];
    gExpensiveNormal = gBufferTextures[1];
    gDepth = gBufferTextures[2];
    gExtraComponents = gBufferTextures[3];
}

void GLWidget::generatePBRColorBufferTexture(int precision) {
    bprColorTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    bprColorTexture->create();
    bprColorTexture->setFormat(QOpenGLTexture::RGBA32F);
    bprColorTexture->setSize(precision, precision, 1);
    bprColorTexture->setMipLevels(bprColorTexture->maximumMipLevels());
    bprColorTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
    bprColorTexture->setWrapMode(QOpenGLTexture::Repeat);
    bprColorTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    bprColorTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    bprColorTexture->generateMipMaps();
}

void GLWidget::generateCompositeBufferTexture(int precision) {
    compositeTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    compositeTexture->create();
    compositeTexture->setFormat(QOpenGLTexture::RGBA32F);
    compositeTexture->setSize(precision, precision, 1);
    compositeTexture->setMipLevels(compositeTexture->maximumMipLevels());
    compositeTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
    compositeTexture->setWrapMode(QOpenGLTexture::Repeat);
    compositeTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    compositeTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    compositeTexture->generateMipMaps();
}

void GLWidget::generateSSRBufferTexture(int precision) {
    ssrTexture.resize(2);

    for (auto & ssrTex : ssrTexture) {
        ssrTex = new QOpenGLTexture(QOpenGLTexture::Target2D);
        ssrTex->create();
        ssrTex->setFormat(QOpenGLTexture::RGBA32F);
        ssrTex->setSize(precision, precision, 1);
        ssrTex->setMipLevels(ssrTex->maximumMipLevels());
        ssrTex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
        ssrTex->setWrapMode(QOpenGLTexture::Repeat);
        ssrTex->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        ssrTex->setMagnificationFilter(QOpenGLTexture::Nearest);
        ssrTex->generateMipMaps();
    }
}

void GLWidget::generateTAABufferTexture(int precision) {
    TAATexture.resize(2);

    for (auto & TAATex : TAATexture) {
        TAATex = new QOpenGLTexture(QOpenGLTexture::Target2D);
        TAATex->create();
        TAATex->setFormat(QOpenGLTexture::RGBA32F);
        TAATex->setSize(precision, precision, 1);
        TAATex->setMipLevels(TAATex->maximumMipLevels());
        TAATex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
        TAATex->setWrapMode(QOpenGLTexture::Repeat);
        TAATex->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        TAATex->setMagnificationFilter(QOpenGLTexture::Nearest);
        TAATex->generateMipMaps();
    }
}

void GLWidget::renderEnvCubeMap(int precision) {
    // rendering
    SHADER(2)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(2)->setUniformValue("equirectangularMap", 0);
    hdrTexture->bind();
    SHADER(2)->setUniformValue("projection", captureProjection);

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    for(int i=0; i<captureViews.count(); ++i) {
        SHADER(2)->setUniformValue("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubeMap->textureId(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cubeGeometry->drawGeometry(
                SHADER(2), hdrTexture);
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    envCubeMap->bind();
    envCubeMap->generateMipMaps();
    envCubeMap->release();
}

void GLWidget::renderIrradianceMap(int precision) {
    // rendering
    SHADER(3)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(3)->setUniformValue("environmentMap", 0);
    envCubeMap->bind();
    SHADER(3)->setUniformValue("projection", captureProjection);

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    for(int i=0; i<captureViews.count(); ++i) {
        SHADER(3)->setUniformValue("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->textureId(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cubeGeometry->drawGeometry(
                SHADER(3), envCubeMap);
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void GLWidget::renderPrefilterMap(int precision) {
    SHADER(4)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(4)->setUniformValue("environmentMap", 0);
    envCubeMap->bind();
    SHADER(4)->setUniformValue("projection", captureProjection);

    captureFBO->bind();
    unsigned int maxMipLevels = prefilterMap->maximumMipLevels();
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth  = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);

        glViewport(0, 0, mipWidth * devicePixelRatio(), mipHeight * devicePixelRatio());
        float roughness = (float)mip / (float)(maxMipLevels - 1);
        SHADER(4)->setUniformValue("roughness", roughness);
        for (int i=0; i<captureViews.count(); ++i) {
            SHADER(4)->setUniformValue("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->textureId(), mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cubeGeometry->drawGeometry(
                    SHADER(4), envCubeMap);
        }
    }

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void GLWidget::renderBRDFMap(int precision) {
    SHADER(5)->bind();

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BRDFMap->textureId(), 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rectGeometry->drawGeometry(
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

QOpenGLFramebufferObject* GLWidget::createSimpleFBOPointer() {
    QOpenGLFramebufferObject *bufferObject;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    format.setTextureTarget(QOpenGLTexture::Target2D);
    format.setSamples(0);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

    return bufferObject;
}

QOpenGLFramebufferObject *GLWidget::createGBufferFBOPointer() {
    QOpenGLFramebufferObject *bufferObject;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    format.setTextureTarget(QOpenGLTexture::Target2D);
    format.setSamples(0);
    format.setMipmap(true);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

    bufferObject->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gAlbedoSpec->textureId(), 0);

    bufferObject->addColorAttachment(frameBufferSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gExpensiveNormal->textureId(), 0);

    bufferObject->addColorAttachment(frameBufferSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDepth->textureId(), 0);

    bufferObject->addColorAttachment(frameBufferSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gExtraComponents->textureId(), 0);

    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);

    qDebug() << "GBuffer FBO attachment count:" << bufferObject->sizes().size();

    return bufferObject;
}

void GLWidget::createSSRFBOPointer() {
    ssrBuffer.resize(2);

    for (auto & ssrFBO : ssrBuffer) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setTextureTarget(QOpenGLTexture::Target2D);
        format.setSamples(0);

        QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
        ssrFBO = new QOpenGLFramebufferObject(frameBufferSize, format);
    }
}

void GLWidget::createTAAFBOPointer() {
    TAABuffer.resize(2);

    for (auto & TAAFBO : TAABuffer) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setTextureTarget(QOpenGLTexture::Target2D);
        format.setSamples(0);

        QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
        TAAFBO = new QOpenGLFramebufferObject(frameBufferSize, format);
    }
}

void GLWidget::updatePreStatues() {
    preViewMatrix = camera->getCameraView();
    isMoving = false;
    update();
}

void GLWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    qDeleteAll(albedo_textures);
    albedo_textures.clear();
    qDeleteAll(metallic_textures);
    metallic_textures.clear();
    qDeleteAll(roughness_textures);
    roughness_textures.clear();
    qDeleteAll(ao_textures);
    ao_textures.clear();
    qDeleteAll(normal_textures);
    normal_textures.clear();

    delete camera;
    delete hdrTexture;
    delete envCubeMap;
    delete irradianceMap;
    delete prefilterMap;
    delete BRDFMap;
    delete captureFBO;
    delete cubeGeometry;
    delete skyboxGeometry;
    delete rectGeometry;
    delete BackDrop;
    delete ShaderBall;
    delete gBuffer;
    delete noiseTexture;

    camera = nullptr;
    hdrTexture = nullptr;
    envCubeMap = nullptr;
    irradianceMap = nullptr;
    prefilterMap = nullptr;
    BRDFMap = nullptr;
    captureFBO = nullptr;
    cubeGeometry = nullptr;
    skyboxGeometry = nullptr;
    rectGeometry = nullptr;
    BackDrop = nullptr;
    ShaderBall = nullptr;
    gBuffer = nullptr;
    noiseTexture = nullptr;

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
        isMoving = true;
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraZoomEvent(offset);

        // update viewport
        update();
        isMoving = true;
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::MidButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraTranslateEvent(offset);

        // update viewport
        update();
        isMoving = true;
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