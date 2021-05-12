#include "mainwidget.h"
#include <QKeyEvent>
#include <QImageReader>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          hdrTexture(nullptr),
          envCubemap(nullptr),
          irradianceMap(nullptr),
          prefilterMap(nullptr),
          debugTexture(nullptr),
          debugSkybox_texture(nullptr),
          brdfLUTTexture(nullptr),
          captureFBO(nullptr),
          cubeGeometry(nullptr),
          skybox_geometry(nullptr),
          sphereGeometry(nullptr),
          QuadGeometry(nullptr),
          customGeometry(nullptr),
          camera(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<7; i++) {
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
    loadDebugCubeMap();
    // --------------------------------------- //

    glSetting();
    initGeometry();

    captureFBO = createFBOPointer(0);
    renderEnvCubeMap(512);
    renderIrradianceMap(128);
    renderPrefilterMap(128);
    renderBRDFMap(512);
}

void MainWidget::paintGL() {

    QOpenGLFramebufferObject::bindDefault();

    SHADER(3)->bind();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Render balls
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

    SHADER(3)->setUniformValue("camPos", camera->getCameraPosition());

    glActiveTexture(GL_TEXTURE0);
    SHADER(3)->setUniformValue("irradianceMap", 0);
    irradianceMap->bind();

    glActiveTexture(GL_TEXTURE1);
    SHADER(3)->setUniformValue("prefilterMap", 1);
    prefilterMap->bind();

    glActiveTexture(GL_TEXTURE2);
    SHADER(3)->setUniformValue("brdfLUT", 2);
    brdfLUTTexture->bind();

    for (int i=0; i<lightPositions.count(); i++) {
        QString lightPos = QString("lightPositions[") + QString::number(i) + QString("]");
        QString lightCol =QString("lightColors[") + QString::number(i) + QString("]");
        SHADER(3)->setUniformValue(lightPos.toStdString().c_str(), lightPositions[i]);
        SHADER(3)->setUniformValue(lightCol.toStdString().c_str(), lightColors[i]);
    }

    SHADER(3)->bind();
    model.setToIdentity();
    model.rotate(75,0,1,0);

    glActiveTexture(GL_TEXTURE3);
    SHADER(3)->setUniformValue("albedoMap", 3);
    albedo_textures[0]->bind();

    glActiveTexture(GL_TEXTURE4);
    SHADER(3)->setUniformValue("metallicMap", 4);
    metallic_textures[0]->bind();

    glActiveTexture(GL_TEXTURE5);
    SHADER(3)->setUniformValue("roughnessMap", 5);
    roughness_textures[0]->bind();

    glActiveTexture(GL_TEXTURE6);
    SHADER(3)->setUniformValue("aoMap", 6);
    ao_textures[0]->bind();

    glActiveTexture(GL_TEXTURE7);
    SHADER(3)->setUniformValue("normalMap", 7);
    normal_textures[0]->bind();

    customGeometry->drawGeometry(
            SHADER(3),
            model,
            camera->getCameraView(),
            camera->getCameraProjection());

    // Render background
    SHADER(5)->bind();
    glActiveTexture(GL_TEXTURE0);
    SHADER(5)->setUniformValue("map", 0);
    envCubemap->bind();
    glDepthFunc(GL_LEQUAL);
    skybox_geometry->drawGeometry(
            SHADER(5),
            camera->getCameraView(),
            camera->getCameraProjection());
    envCubemap->release();
    glDepthFunc(GL_LESS);
}

void MainWidget::resizeGL(int width, int height) {
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

void MainWidget::initShaders() {
    // for cube box
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/equirectangular_to_map.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    // irradiance
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/irradiance_convolution.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    // prefilter
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/cubemap.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/prefilter.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();

    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/pbr.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/pbr.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
    if (!SHADER(3)->bind())
        close();

    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/brdf.vs.glsl"))
        close();
    if (!SHADER(4)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/brdf.fs.glsl"))
        close();
    if (!SHADER(4)->link())
        close();
    if (!SHADER(4)->bind())
        close();

    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/16_IBL_Models/shaders/background.vs.glsl"))
        close();
    if (!SHADER(5)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/16_IBL_Models/shaders/background.fs.glsl"))
        close();
    if (!SHADER(5)->link())
        close();
    if (!SHADER(5)->bind())
        close();

    // Debug shader
    if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/ScreenPlane.vs.glsl"))
        close();
    if (!SHADER(6)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/ScreenPlane.fs.glsl"))
        close();
    if (!SHADER(6)->link())
        close();
    if (!SHADER(6)->bind())
        close();
}

void MainWidget::initGeometry() {
    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(0));

    sphereGeometry = new SphereGeometry;
    sphereGeometry->initGeometry();
    sphereGeometry->setupAttributePointer(SHADER(3));

    QuadGeometry = new RectangleGeometry;
    QuadGeometry->initGeometry();
    QuadGeometry->setupAttributePointer(SHADER(4));

    skybox_geometry = new SkyboxGeometry;
    skybox_geometry->initGeometry();
    skybox_geometry->setupAttributePointer(SHADER(5));

    customGeometry = new CustomGeometry(QString("src/resource/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX"));
    customGeometry = new CustomGeometry(QString("src/resource/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX"));
    customGeometry->initGeometry();
    customGeometry->setupAttributePointer(SHADER(3));
}

void MainWidget::initTexture() {
    debugTexture = new QOpenGLTexture(QImage(QString("src/texture/wall.jpg")).mirrored(false, true));

    loadHDRTextrue();
    loadMaterialTextures();
    generateEnvCubeMap(512);
    generateIrradianceMap(128);
    generatePrefilterMap(128);
    generateBRDFMap(512);
}

void MainWidget::loadMaterialTextures() {
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
        alebdoTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
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

void MainWidget::loadHDRTextrue() {
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

void MainWidget::loadDebugCubeMap() {
    const QImage posx = QImage(faces[0]).convertToFormat(QImage::Format_RGBA8888);
    const QImage negx = QImage(faces[1]).convertToFormat(QImage::Format_RGBA8888);
    const QImage posy = QImage(faces[2]).convertToFormat(QImage::Format_RGBA8888);
    const QImage negy = QImage(faces[3]).convertToFormat(QImage::Format_RGBA8888);
    const QImage posz = QImage(faces[4]).convertToFormat(QImage::Format_RGBA8888);
    const QImage negz = QImage(faces[5]).convertToFormat(QImage::Format_RGBA8888);

    debugSkybox_texture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    debugSkybox_texture->create();
    debugSkybox_texture->setSize(posx.width(), posx.height(), posx.depth());
    debugSkybox_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    debugSkybox_texture->setMipLevels(debugSkybox_texture->maximumMipLevels());
    debugSkybox_texture->allocateStorage();

    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posx.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negx.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posy.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negy.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posz.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negz.constBits(), Q_NULLPTR);

    debugSkybox_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    debugSkybox_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    debugSkybox_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    debugSkybox_texture->generateMipMaps();
}

void MainWidget::generateEnvCubeMap(int precision) {
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

void MainWidget::generateIrradianceMap(int precision) {
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

void MainWidget::generatePrefilterMap(int precision) {
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

void MainWidget::generateBRDFMap(int precision) {
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

void MainWidget::renderEnvCubeMap(int precision) {
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

void MainWidget::renderIrradianceMap(int precision) {
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

void MainWidget::renderPrefilterMap(int precision) {
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

void MainWidget::renderBRDFMap(int precision) {
    SHADER(4)->bind();

    glViewport(0, 0, precision * devicePixelRatio(), precision * devicePixelRatio());
    captureFBO->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture->textureId(), 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QuadGeometry->drawGeometry(
            SHADER(4));

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
}

void MainWidget::glSetting() {
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

QOpenGLFramebufferObject* MainWidget::createFBOPointer(int sampleCount) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(QOpenGLTexture::RGB32F);
    format.setSamples(sampleCount);
    format.setMipmap(true);

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    return new QOpenGLFramebufferObject(frameBufferSize, format);
}

void MainWidget::cleanup() {
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
    delete envCubemap;
    delete irradianceMap;
    delete prefilterMap;
    delete brdfLUTTexture;
    delete debugTexture;
    delete debugSkybox_texture;
    delete captureFBO;
    delete cubeGeometry;
    delete skybox_geometry;
    delete sphereGeometry;
    delete QuadGeometry;
    delete customGeometry;

    camera = nullptr;
    hdrTexture = nullptr;
    envCubemap = nullptr;
    irradianceMap = nullptr;
    prefilterMap = nullptr;
    brdfLUTTexture = nullptr;
    debugTexture = nullptr;
    debugSkybox_texture = nullptr;
    captureFBO = nullptr;
    cubeGeometry = nullptr;
    skybox_geometry = nullptr;
    sphereGeometry = nullptr;
    QuadGeometry = nullptr;
    customGeometry = nullptr;

    doneCurrent();
}

void MainWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        if (DEBUG) {
            qDebug() << "Alt + LeftButton";
        }
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
        if (DEBUG) {
            qDebug() << "Alt + MidButton";
        }
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
        if (DEBUG) {
            qDebug() << "Mouse offset x:" << offset.x();
            qDebug() << "Mouse offset y:" << offset.y();
        }

        camera->cameraRotateEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        if (DEBUG) {
            qDebug() << "Mouse offset x:" << offset.x();
        }

        camera->cameraZoomEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::MidButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        if (DEBUG) {
            qDebug() << "MidButton Press Mouse offset x:" << offset.x();
            qDebug() << "MidButton Press Mouse offset y:" << offset.y();
        }

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

    if (DEBUG)
        qDebug() << "Current fov:" << fov;

    QWidget::wheelEvent(event);
}
