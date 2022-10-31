#ifndef INHOUSE_QTOPENGL_HELLOWINDOW_H
#define INHOUSE_QTOPENGL_HELLOWINDOW_H

#include <QtOpenGL/QOpenGLWindow>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLBuffer>
#include <QtOpenGL/QOpenGLTexture>
#include <QtOpenGL/QOpenGLVertexArrayObject>

#include <QTimer>

#ifndef __EMSCRIPTEN__
    #include <QtOpenGL/QOpenGLFunctions_4_5_Core>
#endif

struct VertexData
{
    QVector3D position;
    QVector3D color;
    QVector2D coord;
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

private:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initShaders();
    void initTextures();
    void initGeometry();

    void rotateAnimate();

private:
    QOpenGLShaderProgram *m_program;
    QOpenGLTexture *texture;

    QVector<VertexData> vertices;
    QVector<GLushort> indices;
    QMatrix4x4 projection;
    QQuaternion rotation;

    QTimer *timer;
    float angle;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer arrayBuf; // Vertex Buffer, vbo
    QOpenGLBuffer indexBuf; // Index Buffer, ebo

    int vertexLocation;
    int colorLocation;
    int coordLocation;
};

#endif
