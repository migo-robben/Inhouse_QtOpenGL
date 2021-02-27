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

class PMainWidget: public QOpenGLWidget {
public:
    explicit PMainWidget(QWidget *parent=nullptr);
    ~PMainWidget() override;

protected:
    QSize sizeHint() const override;

public:
    virtual void initShaders()=0;
    virtual void initPlaneGeometry()=0;
    virtual void initTextures()=0;
    virtual QVector<VertexData> getVerticesData()=0;
    virtual QVector<GLushort> getIndices()=0;
};


#endif //INHOUSE_QTOPENGL_PMAINWIDGET_H
