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
          diffuseTexture(nullptr){

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

    createBlendShapeTex();

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
    SHADER(0)->setUniformValue("iScaleFactor", iScaleFactor);

    for(int i=0;i < customGeometry->m_NumBlendShape;i++){
        glActiveTexture(GL_TEXTURE0);
        QString bsMap = QString("blendShapeMap") + QString::number(i);
        SHADER(0)->setUniformValue("blendShapeMap1", 0);
        blendShapeTextures[i]->bind();
    }

    model.setToIdentity();
    model.translate(QVector3D(0.0, -1.0, 0.0));
    model.scale(1);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    customGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection());
}

void GLWidget::createBlendShapeTex() {
    int lengthBSD = customGeometry->m_blendShapeData.length();
    int bsNum = customGeometry->m_NumBlendShape;
    iScaleFactor = std::pow(2, int(customGeometry->scaleFactor/2) + 1);
//    iScaleFactor = 1;
    qDebug() << "lengthBSD: " << lengthBSD;
    qDebug() << "iScaleFactor: " << iScaleFactor;
    for(int i=0; i<bsNum;i++){
        QOpenGLTexture* blendShapeTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        blendShapeTexture->create();
        blendShapeTexture->setFormat(QOpenGLTexture::RGBA32F);
        blendShapeTexture->setSize(precision, precision, 3);
        blendShapeTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);

        QVector<QVector4D> bsData;
        for(int j=0; j<precision*precision; j++){
            QVector4D verData;
            if(j<lengthBSD){
                QVector3D deltaPos = (customGeometry->m_blendShapeData[j].m_AnimDeltaPos[i] / float(iScaleFactor) + QVector3D(1.0, 1.0, 1.0)) / 2.0;
                verData = QVector4D(deltaPos, 1.0f);
            }else{
                verData = QVector4D(0.0, 0.0, 0.0, 1.0);
            }
            bsData.append(verData);
        }
        blendShapeTexture->setData(0, QOpenGLTexture::RGBA,QOpenGLTexture::Float32, bsData.constData());

        blendShapeTextures.push_back(blendShapeTexture);
    }
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