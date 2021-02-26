#ifndef _INHOUSE_OPENGL_VAO_TEXTURE_
#define _INHOUSE_OPENGL_VAO_TEXTURE_

#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>

struct VertexData
{
    QVector3D position;
    QVector3D color;
    QVector2D coord;
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
    void initTextures();

    QVector<VertexData> getVerticesData();
    QVector<GLushort> getIndices();

private:
    QOpenGLShaderProgram program;
    QOpenGLTexture *texture{};

    QMatrix4x4 projection;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo; // Vertex Buffer
    QOpenGLBuffer ebo; // Index Buffer

private:
    QVector<VertexData> vertices;
    QVector<GLushort> indices;

    int vertexLocation{};
    int colorLocation{};
    int coordLocation{};
};


#endif
