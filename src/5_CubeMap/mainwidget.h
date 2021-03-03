#ifndef INHOUSE_QTOPENGL_MAINWIDGET_H
#define INHOUSE_QTOPENGL_MAINWIDGET_H

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
#include "Helper/SkyboxGeometry.h"

class CubeGeometry;
class SkyboxGeometry;
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
    void loadCubeMap();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram*> programs;
    SkyboxGeometry *skyboxGeometry;
    CubeGeometry *cubeGeometry;
    QOpenGLTexture *skyboxTexture;
    QOpenGLTexture *containerTexture;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QList<QString> faces{
            QString("src/texture/CubeMap/right.jpg"),
            QString("src/texture/CubeMap/left.jpg"),
            QString("src/texture/CubeMap/top.jpg"),
            QString("src/texture/CubeMap/bottom.jpg"),
            QString("src/texture/CubeMap/front.jpg"),
            QString("src/texture/CubeMap/back.jpg"),
    };

public slots:
    void cleanup();
};


#endif
