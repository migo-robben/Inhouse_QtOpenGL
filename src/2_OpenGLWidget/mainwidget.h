#ifndef INHOUSE_QTOPENGL_MAINWIDGET_H
#define INHOUSE_QTOPENGL_MAINWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_5_Core>

struct VertexData
{
    QVector3D position;
    QVector3D color;
};

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
public:
    explicit MainWidget(QWidget *parent=nullptr);
    ~MainWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    QSize sizeHint() const override;

    void initShaders();
    void initPlaneGeometry();

    QVector<VertexData> getVerticesData();
    QVector<GLushort> getIndices();

private:
    QOpenGLShaderProgram program;
    QMatrix4x4 projection;
    QQuaternion rotation;

    QOpenGLBuffer arrayBuf; // Vertex Buffer
    QOpenGLBuffer indexBuf; // Index Buffer

private:
    QVector<VertexData> vertices;
    QVector<GLushort> indices;

    int vertexLocation;
    int colorLocation;
};


#endif
