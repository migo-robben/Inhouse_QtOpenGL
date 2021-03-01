#ifndef INHOUSE_QTOPENGL_OPENGLCAMERAMAINWIDGET_H
#define INHOUSE_QTOPENGL_OPENGLCAMERAMAINWIDGET_H

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>

#include <QDebug>

#include "Helper/CubeGeometry.h"
#include "Helper/Camera.h"

class CubeGeometry;
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

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

private:
    QOpenGLShaderProgram *program;
    QOpenGLTexture *texture;
    CubeGeometry *geometry;
    Camera *camera;

    QMatrix4x4 model;

    QPoint mousePos;

public slots:
    void cleanup();

};

#endif
