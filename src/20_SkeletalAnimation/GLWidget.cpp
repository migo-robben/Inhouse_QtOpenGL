#include "GLWidget.h"
#include <QKeyEvent>
#include <QTimer>

//#define STB_IMAGE_IMPLEMENTATION
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

    createBlendShapeTex(true);
    createUDIMTex();

    if(customGeometry->m_animationNum){
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &GLWidget::updateFrame);
        int refreshMs = 1000.0 / customGeometry->animation.getTicksPerSecond();
        qDebug() << "refresh: " << refreshMs;
        timer->start(refreshMs);
        elapsedTimer.start();
    }
}

void GLWidget::paintGL() {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SHADER(0)->bind();

    SHADER(0)->setUniformValue("NumAnimation", customGeometry->m_animationNum);
    float currentTime = elapsedTimer.elapsed() / 1000.0;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    customGeometry->animator.updateAnimation(deltaTime);
    QVector<QMatrix4x4> transforms = customGeometry->animator.getPoseTransforms();
    SHADER(0)->setUniformValueArray("finalBonesMatrices", transforms.data(), transforms.size());
    //qDebug() << transforms;
    //qDebug() << customGeometry->animator.bsWeights.count();
    SHADER(0)->setUniformValueArray("BlendShapeWeight", customGeometry->animator.bsWeights.data(), customGeometry->animator.bsWeights.count());
    //qDebug() << "BlendShapeWeights: " << customGeometry->animator.bsWeights.count() << customGeometry->animator.bsWeights;
    SHADER(0)->setUniformValue("NumBlendShapeWeight", customGeometry->animator.bsWeights.count());
    SHADER(0)->setUniformValue("ScaleFactorX", scaleFactorX);
    SHADER(0)->setUniformValue("ScaleFactorY", scaleFactorY);
    SHADER(0)->setUniformValue("ScaleFactorZ", scaleFactorZ);
    SHADER(0)->setUniformValueArray("BlendShapeSlice", customGeometry->blendShapeSlice.data(), customGeometry->blendShapeSlice.count());

    SHADER(0)->setUniformValueArray("UdimQuadrant", udimQuadrant.data(), udimQuadrant.count());
    SHADER(0)->setUniformValue("NumUdimQuadrant", udimQuadrant.count());

    diffuseUDIMTex->bind(0);
    SHADER(0)->setUniformValue("diffuseUDIM", 0);

    for (int i=0; i<blendShapeTexs.count(); i++) {
        QString bsMap = QString("blendShapeMap[") + QString::number(i) + QString("]");
        blendShapeTexs[i]->bind(i);
        SHADER(0)->setUniformValue(bsMap.toStdString().c_str(), i);
    }

    model.setToIdentity();
    model.translate(QVector3D(0.0, -1.0, 0.0));
    model.scale(0.1);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    customGeometry->drawGeometry(
            SHADER(0),
            model,
            camera->getCameraView(),
            camera->getCameraProjection());
}

void GLWidget::createUDIMTex(){
    QVector<QImage> diffuseUIDMTexs;
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1001.png")).convertToFormat(QImage::Format_RGBA8888));
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1002.png")).convertToFormat(QImage::Format_RGBA8888));
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1003.png")).convertToFormat(QImage::Format_RGBA8888));
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1011.png")).convertToFormat(QImage::Format_RGBA8888));
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1012.png")).convertToFormat(QImage::Format_RGBA8888));
    diffuseUIDMTexs.push_back(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure32bit_1023.png")).convertToFormat(QImage::Format_RGBA8888));

    udimQuadrant.push_back(1001);
    udimQuadrant.push_back(1002);
    udimQuadrant.push_back(1003);
    udimQuadrant.push_back(1011);
    udimQuadrant.push_back(1012);
    udimQuadrant.push_back(1023);

    diffuseUDIMTex = new QOpenGLTexture(QOpenGLTexture::Target::Target2DArray);
    diffuseUDIMTex->create();
    diffuseUDIMTex->setFormat(QOpenGLTexture::SRGB8);
    diffuseUDIMTex->setSize(512, 512, 3);
    diffuseUDIMTex->setLayers(6);
    diffuseUDIMTex->allocateStorage();

    for(int i=0;i<6;i++){
        diffuseUDIMTex->setData(0, i,QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, diffuseUIDMTexs[i].constBits());
    }
}

void GLWidget::createBlendShapeTex(bool write2disk) {
    int bsDataLength = customGeometry->m_BSDATA.length();
    qDebug() << "********** BlendShape Tex **********";
    qDebug() << "BlendShapeDataLength: " << bsDataLength;

    assert(bsDataLength == customGeometry->m_BSID);

    for(int z=0;z<customGeometry->m_BSDATA.length();z++){
        QVector<BlendShapePosition> currentBsData = customGeometry->m_BSDATA[z];
        int currLenBsVertex = currentBsData.length();
        int currBsNum = currentBsData[0].m_numAnimPos;
        int texDepth = 4;
        int precision = CustomGeometry::computeLevelByVCount(currLenBsVertex, 2);

        QOpenGLTexture* blendShapeTex = new QOpenGLTexture(QOpenGLTexture::Target::Target2DArray);
        blendShapeTex->create();
        blendShapeTex->setFormat(QOpenGLTexture::RGBA32F);
        blendShapeTex->setSize(precision, precision, 3);
        blendShapeTex->setLayers(currBsNum);
        blendShapeTex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);

        scaleFactorX = std::pow(2, int(std::round(customGeometry->scaleFactor.x()/2.0f) + 1));
        scaleFactorY = std::pow(2, int(std::round(customGeometry->scaleFactor.y()/2.0f) + 1));
        scaleFactorZ = std::pow(2, int(std::round(customGeometry->scaleFactor.z()/2.0f) + 1));

        qDebug() << "precision: " << precision;
        qDebug() << "scaleFactor: " << scaleFactorX << scaleFactorY << scaleFactorZ;
        qDebug() << "currLenBsVertex: " << currLenBsVertex << " currBsNum: " << currBsNum;
        for(int i=0; i<currBsNum;i++){
            int halfIndex = precision*precision/2;
            QVector<unsigned char> bsDataUc;
            QVector<QVector4D> bsData;

            bsDataUc.resize(precision*precision*texDepth);
            bsDataUc.fill(0);
            bsData.resize(precision*precision);
            QVector4D empty(0.0, 0.0, 0.0, 1.0);
            bsData.fill(empty);

            for(int j=0; j<precision*precision; j++){
                if(j == halfIndex)
                    break;

                QVector4D verData;
                QVector4D norData;
                verData = QVector4D(0.0, 0.0, 0.0, 1.0);
                norData = QVector4D(0.0, 0.0, 0.0, 1.0);
                if(j<currLenBsVertex){
                    QVector3D deltaPos = (QVector3D(currentBsData[j].m_AnimDeltaPos[i].x() / float(scaleFactorX),
                                                    currentBsData[j].m_AnimDeltaPos[i].y() / float(scaleFactorY),
                                                    currentBsData[j].m_AnimDeltaPos[i].z() / float(scaleFactorZ))
                                                            + QVector3D(1.0, 1.0, 1.0)) / 2.0;
                    QVector3D deltaNor = (currentBsData[j].m_AnimDeltaNor[i] + QVector3D(1.0, 1.0, 1.0)) / 2.0;
                    verData = QVector4D(deltaPos, 1.0f);
                    norData = QVector4D(deltaNor, 1.0f);
                }

                bsData[j] = verData;
                bsData[halfIndex+j] = norData;
                // Debug
                if(write2disk) {
                    bsDataUc[j * 4 + 0] = verData.x() * 255;
                    bsDataUc[j * 4 + 1] = verData.y() * 255;
                    bsDataUc[j * 4 + 2] = verData.z() * 255;
                    bsDataUc[j * 4 + 3] = 255;
                    bsDataUc[(halfIndex + j) * 4 + 0] = norData.x() * 255;
                    bsDataUc[(halfIndex + j) * 4 + 1] = norData.y() * 255;
                    bsDataUc[(halfIndex + j) * 4 + 2] = norData.z() * 255;
                    bsDataUc[(halfIndex + j) * 4 + 3] = 255;
                }
            }
            // Write to disk
            if(write2disk){
                QImage image(bsDataUc.data(), precision, precision, QImage::Format_RGBA8888);
                image.save(QString("src/20_SkeletalAnimation/resource/blendShapeTex")
                    +QString::number(z)
                    +QString("_")
                    +QString::number(i)
                    +QString(".png"));
            }
            blendShapeTex->setData(0, i, QOpenGLTexture::RGBA,QOpenGLTexture::Float32, bsData.constData());
        }
        blendShapeTexs.push_back(blendShapeTex);
    }
    qDebug() << "*********************";
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void GLWidget::initShaders() {
    /*
    QFile vsfile("src/20_SkeletalAnimation/shaders/skeletalAnimation.vs.glsl");
    if(!vsfile.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << " Could not open the file";
        return;
    }
    QTextStream in(&vsfile);
    QString shaderData = in.readAll();
    shaderData.replace(QString("@MaxBSNum@"), QString("15"));
    vsfile.close();
    */

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
    customGeometry = new CustomGeometry(QString("src/20_SkeletalAnimation/resource/CHARACTER-letian-adv-_char_letian+Group-test.fbx")); // dancing_vampire
    customGeometry->initGeometry();
    customGeometry->initAnimation();
    customGeometry->initAnimator();
    customGeometry->setupTransformationAttribute();
    customGeometry->initAllocate();
    customGeometry->setupAttributePointer(SHADER(0));
}

void GLWidget::initTexture() {
    diffuseTexture = new QOpenGLTexture(QImage(QString("src/20_SkeletalAnimation/resource/images/Pure24bit_1001.png")));
    //blendShapeTex = new QOpenGLTexture(QOpenGLTexture::Target::Target2DArray);
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