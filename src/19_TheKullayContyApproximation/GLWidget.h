#ifndef _THEKULLAYCONTYAPPROXIMATION_
#define _THEKULLAYCONTYAPPROXIMATION_

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

class Camera;
class CubeGeometry;
class SkyboxGeometry;
class SphereGeometry;
class RectangleGeometry;
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

    void glSetting();
    void loadHDRTextrue();
    void generateEnvCubeMap(int precision);
    void generateIrradianceMap(int precision);
    void generatePrefilterMap(int precision);
    void generateBRDFMap(int precision);
    void renderEnvCubeMap(int precision);
    void renderIrradianceMap(int precision);
    void renderPrefilterMap(int precision);
    void renderBRDFMap(int precision);

    void renderSphere(QOpenGLShaderProgram *shader, float YOffset, bool metal, bool isComputePointLight);
    void renderShadingBall(QOpenGLShaderProgram *shader, float YOffset, bool isComputePointLight);

    QOpenGLFramebufferObject* createFBOPointer(int sampleCount=0);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram *> programs;
    QOpenGLTexture *hdrTexture;
    QOpenGLTexture *envCubemap;
    QOpenGLTexture *irradianceMap;
    QOpenGLTexture *prefilterMap;
    QOpenGLTexture *brdfLUTTexture;

    CubeGeometry *cubeGeometry;
    SkyboxGeometry *skybox_geometry;
    SphereGeometry *sphereGeometry;
    RectangleGeometry *QuadGeometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *captureFBO;

    QMatrix4x4 captureProjection;
    QVector<QMatrix4x4> captureViews;

    bool furnaceTest = false;
    bool computePointLight = true;
    bool environmentCompensation = true;
    bool renderCustomGeo = true;

    QString shadingBallGeoPath = "src/18_ScreenSpaceReflection/Models/ShaderBall.obj";
    CustomGeometry *shadingBallGeo;

    // albedo texture
    QOpenGLTexture* albedo_texture;
    QString albedoTextureFilePath = "src/texture/PBR/gold/albedo.png";

    // metallic texture
    QOpenGLTexture* metallic_texture;
    QString metalTextureFilePath = "src/texture/PBR/gold/metallic.png";

    // roughness texture
    QOpenGLTexture* roughness_texture;
    QString roughnessTextureFilePath = "src/texture/PBR/gold/roughness.png";

    // ao texture
    QOpenGLTexture* ao_texture;
    QString aoTextureFilePath = "src/texture/PBR/gold/ao.png";

    // normal texture
    QOpenGLTexture* normal_texture;
    QString normalTextureFilePath = "src/texture/PBR/gold/normal.png";

    QOpenGLTexture* EavgLUT;
    QString EavgLUTPath = "src/19_TheKullayContyApproximation/image/Eavg_LUT.png";

    // Lights
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

public slots:
    void cleanup();
};

#endif
