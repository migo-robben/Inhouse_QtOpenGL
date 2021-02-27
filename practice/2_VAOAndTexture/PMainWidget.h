#ifndef INHOUSE_QTOPENGL_PMAINWIDGET_H
#define INHOUSE_QTOPENGL_PMAINWIDGET_H
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>

struct VertexData
{
    QVector3D position;
    QVector3D color;
    QVector2D coord;
};

class PMainWidget: public QOpenGLWidget, QOpenGLFunctions_4_5_Core {
public:
    explicit PMainWidget(QWidget *parent=nullptr);
    ~PMainWidget() override;

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


#endif //INHOUSE_QTOPENGL_PMAINWIDGET_H
