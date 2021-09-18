#ifndef QTREFERENCE_GLWIDGET_SHADOWMAPPCF_H
#define QTREFERENCE_GLWIDGET_SHADOWMAPPCF_H

#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QApplication>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include "Helper/Camera.h"
#include "Helper/CubeGeometry.h"
#include "Helper/FloorGeometry.h"
#include "Helper/RectangleGeometry.h"
#include "Helper/CustomGeometry.h"

class CubeGeometry;
class FloorGeometry;
class RectangleGeometry;
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
    void generateDepthMap(int precision);
    void renderScene();

    void renderDepth();
    void renderDepth_OpenGL();

    void renderLight();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

    QOpenGLFramebufferObject* createFBOPointer(int sampleCount);
    void createFBOPointer_OpenGL();
    void configLight();

private:
    QList<QOpenGLShaderProgram*> programs;
    QOpenGLTexture *wood_texture;
    QOpenGLTexture *depth_texture;

    CubeGeometry *cube_geometry;
    FloorGeometry *floor_geometry;
    FloorGeometry *light_geometry;
    RectangleGeometry *rect_geometry;
    CustomGeometry* custom_geometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *fbo;

    unsigned int depthMapFBO;
    unsigned int depthMap;

    QMatrix4x4 lightProjection, lightView;
    QVector3D lightPos;

public slots:
    void cleanup();
};


#endif
