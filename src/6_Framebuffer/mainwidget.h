#ifndef INHOUSE_QTOPENGL_FRAMEBUFFER_MAINWIDGET_H
#define INHOUSE_QTOPENGL_FRAMEBUFFER_MAINWIDGET_H

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include "Helper/Camera.h"
#include "Helper/CubeGeometry.h"
#include "Helper/GridGeometry.h"

class CubeGeometry;
class GridGeometry;
class Camera;

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

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

    void glSetting();

    QOpenGLFramebufferObject* createFBOPointer();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    QList<QOpenGLShaderProgram*> programs;
    CubeGeometry *cubeGeometry;
    GridGeometry *gridGeometry;
    QOpenGLTexture *containerTexture;
    QOpenGLTexture *gridTexture;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *fbo;
    bool justForDebug;

public slots:
    void cleanup();
};

#endif
