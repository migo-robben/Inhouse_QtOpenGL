#ifndef _IBLSPECULARGLWIDGET_H_
#define _IBLSPECULARGLWIDGET_H_

#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QtWidgets/QApplication>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include <QDebug>

#include "Helper/Camera.h"
#include "Helper/CubeGeometry.h"
#include "Helper/SkyboxGeometry.h"
#include "Helper/SphereGeometry.h"
#include "Helper/RectangleGeometry.h"

#ifndef DEBUG
#define DEBUG false
#endif

class Camera;
class CubeGeometry;
class SkyboxGeometry;
class SphereGeometry;
class RectangleGeometry;

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
    void loadHDRTextrue();
    void loadDebugCubeMap();
    void generateEnvCubeMap(int precision);
    void generateIrradianceMap(int precision);
    void generatePrefilterMap(int precision);
    void generateBRDFMap(int precision);
    void renderEnvCubeMap(int precision);
    void renderIrradianceMap(int precision);
    void renderPrefilterMap(int precision);
    void renderBRDFMap(int precision);

    QOpenGLFramebufferObject* createFBOPointer(int sampleCount=0);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram *> programs;
    QOpenGLTexture *hdrTexture;
    QOpenGLTexture *envCubemap;
    QOpenGLTexture *debugSkybox_texture;
    QOpenGLTexture *irradianceMap;
    QOpenGLTexture *prefilterMap;
    QOpenGLTexture *brdfLUTTexture;
    QOpenGLTexture *debugTexture;

    CubeGeometry *cubeGeometry;
    SkyboxGeometry *skybox_geometry;
    SphereGeometry *sphereGeometry;
    RectangleGeometry *QuadGeometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *captureFBO;

    QList<QString> faces{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/right.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/left.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/top.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/bottom.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/front.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/back.jpg"),
    };

    QMatrix4x4 captureProjection;
    QVector<QMatrix4x4> captureViews;

public slots:
    void cleanup();
};


#endif
