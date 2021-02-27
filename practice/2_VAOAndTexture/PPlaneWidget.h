#ifndef INHOUSE_QTOPENGL_PPLANEWIDGET_H
#define INHOUSE_QTOPENGL_PPLANEWIDGET_H

#include "PMainWidget.h"

class PPlaneWidget : public PMainWidget, QOpenGLFunctions_4_5_Core{

public:
    explicit PPlaneWidget(QWidget *widget=nullptr);
    ~PPlaneWidget();
    QVector<VertexData> getVerticesData() override;
    QVector<GLushort> getIndices() override;
    void initShaders() override;
    void initPlaneGeometry() override;
    void initTextures() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QOpenGLShaderProgram program;
    QOpenGLTexture *texture;
    QOpenGLTexture *texture1;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
};


#endif //INHOUSE_QTOPENGL_PPLANEWIDGET_H
