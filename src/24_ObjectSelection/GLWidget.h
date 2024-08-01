#ifndef QTREFERENCE_StencilTest_GLWIDGET_H
#define QTREFERENCE_StencilTest_GLWIDGET_H

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include <QDebug>

#include "Helper/CubeGeometry.h"
#include "Helper/FloorGeometry.h"
#include "Helper/Camera.h"

#include <memory>

class CubeGeometry;
class FloorGeometry;
class Camera;


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

    // ----- Initialization ----- //
    void initShaders();
    void initGeometry();
    void initTexture();

    // ----- Base OpenGL Setting ----- //
    void glSetting();

    // ----- Mouse Event ----- //
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

    // ----- Draw Mode ----- //
    void drawOutline();
    void drawDefaultWithStencil();

    // ----- Selected Object ----- //
    void checkSelected();

private:
    QVector<QOpenGLShaderProgram*> programs;

    // Texture
    std::shared_ptr<QOpenGLTexture> floorTex;
    std::shared_ptr<QOpenGLTexture> cubeTex;

    // Camera
    std::shared_ptr<Camera> camera;

    // Geometry
    std::shared_ptr<CubeGeometry> cube;
    std::shared_ptr<FloorGeometry> floor;

    QPoint mousePos;
    QPoint mouseSelectedPos;
    QMatrix4x4 model;

    QOpenGLFramebufferObject* mFBO = nullptr;
    bool selectedObj = false;
    bool check = false;
    GLuint selectedObjectIndex;

public slots:
    void cleanup();
};


#endif
