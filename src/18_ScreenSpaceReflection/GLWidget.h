#ifndef _SSR_H_
#define _SSR_H_

#include <QTime>
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
#include "Helper/CustomGeometry.h"
#include "Helper/SkyboxGeometry.h"
#include "Helper/SphereGeometry.h"
#include "Helper/RectangleGeometry.h"

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
    void loadHDRTexture();

    // ----- PBR functions ----- //
    void generateEnvCubeMap(int precision);
    void generateIrradianceMap(int precision);
    void generatePrefilterMap(int precision);
    void generateBRDFMap(int precision);

    void renderEnvCubeMap(int precision);
    void renderIrradianceMap(int precision);
    void renderPrefilterMap(int precision);
    void renderBRDFMap(int precision);

    // ----- SSR functions ----- //
    void generateGBufferTexture(int precision);
    void generatePBRColorBufferTexture(int precision);
    void generateSSRBufferTexture(int precision);
    void generateTAABufferTexture(int precision);
    void generateCompositeBufferTexture(int precision);

    void loadMaterialTextures();

    // ----- FrameBuffers ----- //
    QOpenGLFramebufferObject* createFBOPointer(int sampleCount=0);
    QOpenGLFramebufferObject* createGBufferFBOPointer();
    QOpenGLFramebufferObject* createSimpleFBOPointer();
    void createSSRFBOPointer();
    void createTAAFBOPointer();

    // ----- Update PreFrame ---- //
    void updatePreStatues();
    QMatrix4x4 preViewMatrix;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram *> programs;
    QOpenGLTexture *hdrTexture;
    QOpenGLTexture *envCubeMap;
    QOpenGLTexture *irradianceMap;
    QOpenGLTexture *prefilterMap;
    QOpenGLTexture *BRDFMap;
    QOpenGLTexture *noiseTexture;

    QOpenGLTexture *gAlbedoSpec, *gExpensiveNormal, *gDepth, *gExtraComponents, *bprColorTexture, *compositeTexture;
    QVector<QOpenGLTexture*> gBufferTextures;
    QVector<QOpenGLTexture*> ssrTexture;
    QVector<QOpenGLTexture*> TAATexture;

    CubeGeometry *cubeGeometry;
    SkyboxGeometry *skyboxGeometry;
    RectangleGeometry *rectGeometry;
    CustomGeometry *BackDrop;
    CustomGeometry *ShaderBall;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *captureFBO;
    QOpenGLFramebufferObject *gBuffer, *pbrBuffer, *compositeBuffer;
    QVector<QOpenGLFramebufferObject*> ssrBuffer;
    bool CurrentSSR = false;
    QVector<QOpenGLFramebufferObject*> TAABuffer;
    bool TAACurrentBuffer = false;

    bool isMoving = false; // camera is moving ?
    int TotalFrames = 0;
    QTime myTimer;

    QList<QString> faces{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/right.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/left.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/top.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/bottom.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/front.jpg"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/CubeMap/back.jpg")
    };

    QMatrix4x4 captureProjection;
    QVector<QMatrix4x4> captureViews;

    // albedo texture
    QList<QOpenGLTexture*> albedo_textures;
    QList<QString> albedoTextureFilePath{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/beatenMetal/beatenMetal-albedo.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/rusted_iron/albedo.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/plastic/albedo.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/gold/albedo.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/wood/albedo.png")
    };

    // metallic texture
    QList<QOpenGLTexture*> metallic_textures;
    QList<QString> metalTextureFilePath{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/beatenMetal/beatenMetal-Metallic.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/rusted_iron/metallic.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/plastic/metallic.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/gold/metallic.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/wood/metal.png")
    };

    // roughness texture
    QList<QOpenGLTexture*> roughness_textures;
    QList<QString> roughnessTextureFilePath{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/beatenMetal/beatenMetal-Roughness.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/rusted_iron/roughness.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/plastic/roughness.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/gold/roughness.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/wood/roughness.png")
    };

    // ao texture
    QList<QOpenGLTexture*> ao_textures;
    QList<QString> aoTextureFilePath{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/beatenMetal/beatenMetal-ao.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/rusted_iron/ao.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/plastic/ao.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/gold/ao.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/wood/ao.png")
    };

    // normal texture
    QList<QOpenGLTexture*> normal_textures;
    QList<QString> normalTextureFilePath{
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/beatenMetal/beatenMetal-Normal.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/rusted_iron/normal.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/plastic/normal.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/gold/normal.png"),
            QString("F:/CLionProjects/QtReference/src/17_qopengl_mess/images/PBR/wood/normal.png")
    };

public slots:
    void cleanup();
};


#endif