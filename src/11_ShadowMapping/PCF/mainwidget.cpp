#include "mainwidget.h"
#include <QKeyEvent>
#include <qmath.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SHADER(x) programs[x]
#define RESOLUTION 2048

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          wood_texture(nullptr),
          depth_texture(nullptr),
          cube_geometry(nullptr),
          floor_geometry(nullptr),
          rect_geometry(nullptr),
          camera(nullptr) {

    for (int i=0; i<3; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    // light pos (-8.7, 9.2, 5.05)
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

    configLight();

    // ----- add shader from source file/code ----- //
    initShaders();
    initTexture();
    generateDepthMap(RESOLUTION);
    // --------------------------------------- //

    glSetting();
    initGeometry();

    if (depth_texture == nullptr) {
        qDebug() << "Use source OpenGL function to create frame buffer object";
        createFBOPointer_OpenGL();
    }
    else {
        qDebug() << "Use QtOpenGL function to create frame buffer object";
        fbo = createFBOPointer(0);
    }
}

void MainWidget::paintGL() {
    // render depth
    if (depth_texture == nullptr) {
        renderDepth_OpenGL();
    }
    else
        renderDepth();

    // render Scene
    QOpenGLFramebufferObject::bindDefault();
    renderScene();

    // for debug
    bool drawDebugPlane = false;
    if (drawDebugPlane) {
        glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SHADER(1)->bind();
        SHADER(1)->setUniformValue("near_plane", 0.1f);
        SHADER(1)->setUniformValue("far_plane", 50.0f);

        SHADER(1)->setUniformValue("depthMap", 0);
        glActiveTexture(GL_TEXTURE0);

        if (depth_texture == nullptr) {
            qDebug() << "Use OpenGL function to bind texture";
            glBindTexture(GL_TEXTURE_2D, depthMap);
        }
        else {
            qDebug() << "Use QtOpenGL function to bind texture";
            glBindTexture(GL_TEXTURE_2D, depth_texture->textureId());
        }

        rect_geometry->drawGeometry(SHADER(1));
    }
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/11_ShadowMapping/PCF/Shaders/shadow_mapping_depth.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/11_ShadowMapping/PCF/Shaders/shadow_mapping_depth.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
    if (!SHADER(0)->bind())
        close();

    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/11_ShadowMapping/PCF/Shaders/debug_quad.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/11_ShadowMapping/PCF/Shaders/debug_quad.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();
    if (!SHADER(1)->bind())
        close();

    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/11_ShadowMapping/PCF/Shaders/shadow_mapping.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/11_ShadowMapping/PCF/Shaders/shadow_mapping.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();
    if (!SHADER(2)->bind())
        close();
}

void MainWidget::initGeometry() {
    cube_geometry = new CubeGeometry;
    cube_geometry->initGeometry();
    cube_geometry->setupAttributePointer(SHADER(0));
    cube_geometry->setupAttributePointer(SHADER(2));

    floor_geometry = new FloorGeometry;
    floor_geometry->initGeometry();
    floor_geometry->setupAttributePointer(SHADER(0));
    floor_geometry->setupAttributePointer(SHADER(2));

    rect_geometry = new RectangleGeometry;
    rect_geometry->initGeometry();
    rect_geometry->setupAttributePointer(SHADER(1));
}

void MainWidget::initTexture() {
    auto imageData = QImage(QString("src/texture/wood.png")).convertToFormat(QImage::Format_RGBA8888);

    wood_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    wood_texture->create();
    wood_texture->setSize(imageData.width(), imageData.height(), imageData.depth());
    wood_texture->setFormat(QOpenGLTexture::SRGB8);
    wood_texture->setMipLevels(wood_texture->maximumMipLevels());
    wood_texture->allocateStorage();
    wood_texture->bind();

    wood_texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageData.constBits());
    wood_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    wood_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    wood_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    wood_texture->generateMipMaps();
}

void MainWidget::glSetting() {
    glEnable(GL_DEPTH_TEST);
}

void MainWidget::generateDepthMap(int precision) {
    depth_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    depth_texture->create();
    depth_texture->setFormat(QOpenGLTexture::D32F);
    depth_texture->setSize(precision, precision, 3);
    depth_texture->allocateStorage(QOpenGLTexture::Depth, QOpenGLTexture::Float32);
    depth_texture->bind();

    depth_texture->setWrapMode(QOpenGLTexture::ClampToBorder);
    depth_texture->setMinificationFilter(QOpenGLTexture::Nearest);
    depth_texture->setMagnificationFilter(QOpenGLTexture::Nearest);

    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void MainWidget::renderScene() {
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SHADER(2)->bind();

    SHADER(2)->setUniformValue("view", camera->getCameraView());
    SHADER(2)->setUniformValue("projection", camera->getCameraProjection());
    SHADER(2)->setUniformValue("lightView", lightView);
    SHADER(2)->setUniformValue("lightProjection", lightProjection);
    SHADER(2)->setUniformValue("lightPos", lightPos);
    SHADER(2)->setUniformValue("viewPos", camera->getCameraPosition());
    SHADER(2)->setUniformValue("MODE", 1);

    SHADER(2)->setUniformValue("diffuseTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wood_texture->textureId());

    SHADER(2)->setUniformValue("shadowMap", 1);
    glActiveTexture(GL_TEXTURE1);
    if (depth_texture == nullptr)
        glBindTexture(GL_TEXTURE_2D, depthMap);
    else
        glBindTexture(GL_TEXTURE_2D, depth_texture->textureId());

    // box 1
    model.setToIdentity();
    model.translate(0.0, 0.5, 0.0);
    model.rotate(45,0,1,0);
    SHADER(2)->setUniformValue("model", model);
    cube_geometry->drawGeometry(SHADER(2));

    // box 2
    model.setToIdentity();
    model.translate(2.5, -0.5, 0);
    model.rotate(-15,0,1,0);
    model.scale(0.5f);
    SHADER(2)->setUniformValue("model", model);
    cube_geometry->drawGeometry(SHADER(2));

    // box 3
    model.setToIdentity();
    model.translate(0.0, 2.0, -3.5);
    model.scale(0.5, 3.0, 0.5);
    SHADER(2)->setUniformValue("model", model);
    cube_geometry->drawGeometry(SHADER(2));

    // floor
    model.setToIdentity();
    model.translate(0, -0.01, 0);
    model.rotate(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 0));
    model.scale(2.0);
    SHADER(2)->setUniformValue("model", model);
    floor_geometry->drawGeometry(
            SHADER(2));
}

void MainWidget::renderDepth() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SHADER(0)->bind();
    glViewport(0, 0, RESOLUTION, RESOLUTION);
    fbo->bind();
    glClear(GL_DEPTH_BUFFER_BIT);

    // box 1
    model.setToIdentity();
    model.translate(0.0, 0.5, 0.0);
    model.rotate(45,0,1,0);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // box 2
    model.setToIdentity();
    model.translate(2.5, -0.5, 0);
    model.rotate(-15,0,1,0);
    model.scale(0.5f);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // box 3
    model.setToIdentity();
    model.translate(0.0, 2.0, -3.5);
    model.scale(0.5, 3.0, 0.5);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // floor
    model.setToIdentity();
    model.translate(0, -0.01, 0);
    model.rotate(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 0));
    model.scale(2.0);
    floor_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    QOpenGLFramebufferObject::bindDefault();
}

void MainWidget::renderDepth_OpenGL() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SHADER(0)->bind();
    glViewport(0, 0, RESOLUTION, RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // box 1
    model.setToIdentity();
    model.translate(0.0, 0.5, 0.0);
    model.rotate(45,0,1,0);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // box 2
    model.setToIdentity();
    model.translate(2.5, -0.5, 0);
    model.rotate(-15,0,1,0);
    model.scale(0.5f);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // box 3
    model.setToIdentity();
    model.translate(0.0, 2.0, -3.5);
    model.scale(0.5, 3.0, 0.5);
    cube_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    // floor
    model.setToIdentity();
    model.translate(0, -0.01, 0);
    model.rotate(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 0));
    model.scale(2.0);
    floor_geometry->drawGeometry(
            SHADER(0),
            model,
            lightView,
            lightProjection,
            wood_texture);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

QOpenGLFramebufferObject* MainWidget::createFBOPointer(int sampleCount) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    format.setInternalTextureFormat(QOpenGLTexture::RGB32F);
    format.setTextureTarget(QOpenGLTexture::Target2D);
    format.setSamples(sampleCount);

    QSize frameBufferSize(RESOLUTION * devicePixelRatio(), RESOLUTION * devicePixelRatio());

    auto bufferObject = new QOpenGLFramebufferObject(frameBufferSize, format);

    bufferObject->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture->textureId(), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    bufferObject->release();

    return bufferObject;
}

void MainWidget::createFBOPointer_OpenGL() {
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, RESOLUTION, RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MainWidget::configLight() {
    lightPos = QVector3D(-2.7f, 3.2f, 2.05f);

    lightProjection.setToIdentity();
    lightView.setToIdentity();

    float near_plane = 0.1f, far_plane = 50.0f;
    qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
    lightProjection.ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    lightView.lookAt(lightPos, QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete wood_texture;
    delete depth_texture;
    delete cube_geometry;
    delete floor_geometry;
    delete rect_geometry;

    camera = nullptr;
    wood_texture = nullptr;
    depth_texture = nullptr;
    cube_geometry = nullptr;
    floor_geometry = nullptr;
    rect_geometry = nullptr;

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
