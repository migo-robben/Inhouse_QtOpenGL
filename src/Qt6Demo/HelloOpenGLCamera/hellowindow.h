#ifndef INHOUSE_QTOPENGL_HELLOWINDOW_H
#define INHOUSE_QTOPENGL_HELLOWINDOW_H

#include <QtOpenGL/QOpenGLWindow>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLBuffer>
#include <QtOpenGL/QOpenGLTexture>
#include <QtOpenGL/QOpenGLVertexArrayObject>

#include <QTimer>
#include <QDebug>

#ifndef __EMSCRIPTEN__
    #include <QtOpenGL/QOpenGLFunctions_4_5_Core>
#endif

#include "Helper/Camera.h"

struct VertexData
{
    QVector3D position;
    QVector2D coord;
    QVector3D normal;
};

class HelloWindow : public QOpenGLWindow
#ifndef __EMSCRIPTEN__
        , protected QOpenGLFunctions_4_5_Core
#endif
{
public:
    HelloWindow();
    ~HelloWindow();

protected:
    QVector<VertexData> getVerticesData();
    QVector<GLushort> getIndices();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;

private:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initShaders();
    void initTextures();
    void initGeometry();

    void rotateAnimate();
    void glSetting();

private:
    QOpenGLShaderProgram *m_program;
    QOpenGLTexture *texture;

    QVector<VertexData> vertices;
    QVector<GLushort> indices;
    QMatrix4x4 projection;
    QQuaternion rotation;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer arrayBuf; // Vertex Buffer, vbo
    QOpenGLBuffer indexBuf; // Index Buffer, ebo

    int vertexLocation;
    int coordLocation;
    int normalLocation;

    Camera *camera;
    QPoint mousePos;
    bool zoomInProcessing = false;
};

#endif
