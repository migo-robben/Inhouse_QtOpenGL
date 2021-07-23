#ifndef _SKELETALANIMATION_
#define _SKELETALANIMATION_

#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QtWidgets/QApplication>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>
#include <QElapsedTimer>
#include <QOpenGLDebugLogger>

#include <QDebug>

#include "Helper/Camera.h"
#include "Helper/CustomGeometry.h"

class Camera;
class CustomGeometry;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool zoomInProcessing = false;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void initShaders();
    void initGeometry();
    void initTexture();
    void updateFrame();

    void glSetting();

    unsigned int scaleFactorX;
    unsigned int scaleFactorY;
    unsigned int scaleFactorZ;
    void createBlendShapeTex(bool);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram*> programs;
    CustomGeometry *customGeometry;
    QOpenGLTexture *diffuseTexture;
    //QOpenGLTexture *blendShapeTex;
    QVector<QOpenGLTexture*> blendShapeTexs;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QElapsedTimer elapsedTimer;
    QTimer *timer;
    float deltaTime = 0.0;
    float lastTime = 0.0;

public slots:
    void cleanup();
};

#endif
