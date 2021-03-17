#include "mainwidget.h"
#include <QKeyEvent>
#include <QTime>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          camera(nullptr),
          sphere(nullptr) {

    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i=0; i<1; i++) {
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
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    // ----- add shader from source file/code ----- //
    initShaders();
    initTexture();
    // --------------------------------------- //

    glSetting();
    initGeometry();
}

void MainWidget::paintGL() {
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    SHADER(0)->setUniformValue("albedo", QVector3D(0.65f, 0.0f, 0.0f));
    SHADER(0)->setUniformValue("ao", 1.0f);
    SHADER(0)->setUniformValue("camPos", camera->getCameraPosition());

    for (int i=0; i<lightPositions.count(); i++) {
        QString lightPos = QString("lightPositions[") + QString::number(i) + QString("]");
        QString lightCol =QString("lightColors[") + QString::number(i) + QString("]");
        SHADER(0)->setUniformValue(lightPos.toStdString().c_str(), lightPositions[i]);
        SHADER(0)->setUniformValue(lightCol.toStdString().c_str(), lightColors[i]);

        model.setToIdentity();
        model.translate(lightPositions[i]);
        sphere->drawGeometry(
                SHADER(0),
                model,
                camera->getCameraView(),
                camera->getCameraProjection());
    }

    int nrRows = 6;
    int nrColumns = 6;
    float spacing = 2.5;
    for (int row = 0; row < nrRows; row++) {
        SHADER(0)->setUniformValue("metallic", float(row)/ float(nrRows));
        for (int col = 0; col < nrColumns; col++) {
            SHADER(0)->setUniformValue("roughness", qBound(0.05f,(float)col / (float)nrColumns,1.0f));
            model.setToIdentity();
            model.translate((col - (nrColumns / 2.0)) * spacing + 1.2f,
                            (row - (nrRows / 2.0)) * spacing + 1.2f,
                            0.0f);

            sphere->drawGeometry(
                    SHADER(0),
                    model,
                    camera->getCameraView(),
                    camera->getCameraProjection());
        }
    }
}

void MainWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    camera->setCameraPerspective(aspect);

    return QOpenGLWidget::resizeGL(width, height);
}

void MainWidget::initShaders() {
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/Shaders/PBRSphere.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/Shaders/PBRSphere.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();
}

void MainWidget::initGeometry() {
    sphere = new SphereGeometry;
    sphere->initGeometry();
    sphere->setupAttributePointer(SHADER(0));
}

void MainWidget::initTexture() {
}

void MainWidget::glSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();
    delete camera;
    delete sphere;

    camera = nullptr;
    sphere = nullptr;

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
