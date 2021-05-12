#ifndef _IBLSPECULARMODELSGLWIDGET_H_
#define _IBLSPECULARMODELSGLWIDGET_H_

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
#include "Helper/CustomGeometry.h"

#ifndef DEBUG
#define DEBUG false
#endif

class Camera;
class CubeGeometry;
class SkyboxGeometry;
class SphereGeometry;
class RectangleGeometry;
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

    void loadMaterialTextures();

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
    CustomGeometry *customGeometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *captureFBO;

    QList<QString> faces{
            QString("src/texture/CubeMap/CubeMap/right.jpg"),
            QString("src/texture/CubeMap/CubeMap/left.jpg"),
            QString("src/texture/CubeMap/CubeMap/top.jpg"),
            QString("src/texture/CubeMap/CubeMap/bottom.jpg"),
            QString("src/texture/CubeMap/CubeMap/front.jpg"),
            QString("src/texture/CubeMap/CubeMap/back.jpg"),
    };

    QMatrix4x4 captureProjection;
    QVector<QMatrix4x4> captureViews;

    // albedo texture
    QList<QOpenGLTexture*> albedo_textures;
    QList<QString> albedoTextureFilePath{
            QString("src/resource/Cerberus_by_Andrew_Maximov/Textures/Cerberus_A.jpg")
    };

    // metallic texture
    QList<QOpenGLTexture*> metallic_textures;
    QList<QString> metalTextureFilePath{
            QString("src/resource/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.jpg")
    };

    // roughness texture
    QList<QOpenGLTexture*> roughness_textures;
    QList<QString> roughnessTextureFilePath{
            QString("src/resource/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.jpg")
    };

    // ao texture
    QList<QOpenGLTexture*> ao_textures;
    QList<QString> aoTextureFilePath{
            QString("src/resource/Cerberus_by_Andrew_Maximov/Textures/Cerberus_AO.jpg")
    };

    // normal texture
    QList<QOpenGLTexture*> normal_textures;
    QList<QString> normalTextureFilePath{
            QString("src/resource/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.jpg")
    };

public slots:
    void cleanup();
};


#endif