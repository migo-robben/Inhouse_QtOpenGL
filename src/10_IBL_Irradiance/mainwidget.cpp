#include "mainwidget.h"
#include <QKeyEvent>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          hdrTexture(nullptr),
          envCubemap(nullptr),
          irradianceMap(nullptr),
          debugSkybox_texture(nullptr),
          captureFBO(nullptr),
          cubeGeometry(nullptr),
          skybox_geometry(nullptr),
          sphereGeometry(nullptr),
          camera(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<4; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    QVector3D cameraPos(0.0, 0.0, 20);
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
}

void MainWidget::paintGL() {

    QOpenGLFramebufferObject::bindDefault();

    SHADER(2)->bind();
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

    SHADER(2)->setUniformValue("albedo", QVector3D(0.65f, 0.0f, 0.0f));
    SHADER(2)->setUniformValue("ao", 1.0f);
    SHADER(2)->setUniformValue("camPos", camera->getCameraPosition());

    glActiveTexture(GL_TEXTURE0);
    SHADER(2)->setUniformValue("irradianceMap", 0);
    irradianceMap->bind();

    for (int i=0; i<lightPositions.count(); i++) {
        QString lightPos = QString("lightPositions[") + QString::number(i) + QString("]");
        QString lightCol =QString("lightColors[") + QString::number(i) + QString("]");
        SHADER(2)->setUniformValue(lightPos.toStdString().c_str(), lightPositions[i]);
        SHADER(2)->setUniformValue(lightCol.toStdString().c_str(), lightColors[i]);

        model.setToIdentity();
        model.translate(lightPositions[i]);
        sphereGeometry->drawGeometry(
                SHADER(2),
                model,
                camera->getCameraView(),
                camera->getCameraProjection());
    }

    int nrRows = 6;
    int nrColumns = 6;
    float spacing = 2.5;
    for (int row = 0; row < nrRows; row++) {
        SHADER(2)->setUniformValue("metallic", float(row)/ float(nrRows));
        for (int col = 0; col < nrColumns; col++) {
            SHADER(2)->setUniformValue("roughness", qBound(0.05f,(float)col / (float)nrColumns,1.0f));
            model.setToIdentity();
            model.translate((col - (nrColumns / 2.0)) * spacing + 1.2f,
                            (row - (nrRows / 2.0)) * spacing + 1.2f,
                            0.0f);

            sphereGeometry->drawGeometry(
                    SHADER(2),
                    model,
                    camera->getCameraView(),
                    camera->getCameraProjection());
        }
    }

    // Render background
    SHADER(3)->bind();
    glDepthFunc(GL_LEQUAL);
    skybox_geometry->drawGeometry(
            SHADER(3),
            camera->getCameraView(),
            camera->getCameraProjection(),
            irradianceMap);
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
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/CubeMap.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/EquirectangularToMap.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    // irradiance
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/CubeMap.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/IrradianceConvolution.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/PBRIrradiance.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/PBRIrradiance.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();

    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/Background.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/Background.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
    if (!SHADER(3)->bind())
        close();
}

void MainWidget::initGeometry() {
    cubeGeometry = new CubeGeometry;
    cubeGeometry->initGeometry();
    cubeGeometry->setupAttributePointer(SHADER(0));

    sphereGeometry = new SphereGeometry;
    sphereGeometry->initGeometry();
    sphereGeometry->setupAttributePointer(SHADER(2));

    skybox_geometry = new SkyboxGeometry;
    skybox_geometry->initGeometry();
    skybox_geometry->setupAttributePointer(SHADER(3));
}

void MainWidget::initTexture() {
    loadHDRTextrue();
    generateEnvCubeMap(512);
    generateIrradianceMap(128);
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
    debugSkybox_texture->allocateStorage();

    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posx.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negx.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posy.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negy.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posz.constBits(), Q_NULLPTR);
    debugSkybox_texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negz.constBits(), Q_NULLPTR);

    debugSkybox_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    debugSkybox_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    debugSkybox_texture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
}

void MainWidget::generateEnvCubeMap(int precision) {
    envCubemap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    envCubemap->create();
    envCubemap->setSize(precision, precision, 3);
    envCubemap->setFormat(QOpenGLTexture::RGB32F);
    envCubemap->allocateStorage();
    envCubemap->bind();

    envCubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    envCubemap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    envCubemap->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
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
    irradianceMap->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
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

void MainWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

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

    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    return new QOpenGLFramebufferObject(frameBufferSize, format);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete hdrTexture;
    delete envCubemap;
    delete irradianceMap;
    delete debugSkybox_texture;
    delete captureFBO;
    delete cubeGeometry;
    delete skybox_geometry;
    delete sphereGeometry;

    camera = nullptr;
    hdrTexture = nullptr;
    envCubemap = nullptr;
    irradianceMap = nullptr;
    debugSkybox_texture = nullptr;
    captureFBO = nullptr;
    cubeGeometry = nullptr;
    skybox_geometry = nullptr;
    sphereGeometry = nullptr;

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
