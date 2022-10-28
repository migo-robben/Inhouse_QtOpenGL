#ifndef INHOUSE_QTOPENGL_HELLOWINDOW_H
#define INHOUSE_QTOPENGL_HELLOWINDOW_H

#include <QtOpenGL/QOpenGLWindow>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLBuffer>

#ifndef __EMSCRIPTEN__
    #include <QtOpenGL/QOpenGLFunctions_4_5_Core>
#endif

struct VertexData
{
    QVector3D position;
    QVector3D color;
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
    void initGeometry();

private:
    QOpenGLShaderProgram *m_program;

    QVector<VertexData> vertices;
    QVector<GLushort> indices;
    QMatrix4x4 projection;

    QOpenGLBuffer arrayBuf; // Vertex Buffer
    QOpenGLBuffer indexBuf; // Index Buffer

    int vertexLocation;
    int colorLocation;
};

#endif
