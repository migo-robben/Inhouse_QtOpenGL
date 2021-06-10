#ifndef _SSAOWIDGET_H_
#define _SSAOWIDGET_H_

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include <QDebug>

#include "Helper/Camera.h"
#include "Helper/CustomGeometry.h"
#include "Helper/RectangleGeometry.h"

class Camera;
class CustomGeometry;
class RectangleGeometry;

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

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

    QOpenGLFramebufferObject* createGBufferFBOPointer();
    QOpenGLFramebufferObject* createSSAOFBOPointer();
    QOpenGLFramebufferObject* createSSAOBlurFBOPointer();

    void getGBufferFBOAttachmentTexture();

    void generateGBufferTexture(int precision);
    void generateSSAOColorBufferTexture(int precision);
    void generateSSAOColorBufferBlurTexture(int precision);
    void generateNoiseTexture();

    void generateSSAOKernelAndSSAONoise();

    float lerp(float a, float b, float f) {
        return a + f * (b - a);
    }

private:
    QList<QOpenGLShaderProgram*> programs;
    CustomGeometry *customGeometry;
    QOpenGLTexture *roomTexture;
    RectangleGeometry *rectGeometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QOpenGLFramebufferObject *gBufferFBO, *ssaoFBO, *ssaoBlurFBO;
    QOpenGLTexture *gPositionDepth, *gNormal, *gAlbedo, *mask, *ssaoColorBuffer, *ssaoColorBufferBlur, *noiseTexture;

    QVector<QVector3D> ssaoKernel, ssaoNoise;
    QVector<GLuint> gBufferTextures;

    bool multiSample = false;

public slots:
    void cleanup();
};

#endif
