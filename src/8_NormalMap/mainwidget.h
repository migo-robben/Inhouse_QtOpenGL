#ifndef INHOUSE_QTOPENGL_MAINWIDGET_NORMALMAP_H
#define INHOUSE_QTOPENGL_MAINWIDGET_NORMALMAP_H

#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QApplication>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include <QDebug>

#include "Helper/Camera.h"
#include "Helper/AxisSystem.h"
#include "Helper/CustomGeometry.h"


class Camera;
class CustomGeometry;

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

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram *> programs;

    CustomGeometry *uiohcfnfa;
    QOpenGLTexture *AlbedoMap;
    QOpenGLTexture *NormalMap;
    QOpenGLTexture *SpecularMap;

    AxisSystem* axisSystem;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

public slots:
    void cleanup();
};


#endif
