#ifndef INHOUSE_QTOPENGL_GLWIDGET_H
#define INHOUSE_QTOPENGL_GLWIDGET_H

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>

#include <QComboBox>

#include <QtDebug>

#include "Helper/GridGeometry.h"

class GridGeometry;

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core{
Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

    QSize sizeHint() const override;

protected:
    void initializeGL() override;
    void paintGL() override;

    void initShaders();
    void initGeometry();
    void initTexture();
    void initBuffers();
    void generateBufferTexture();
    void getBufferTexture();

    void glSetting();

    void createCombobox();
    void createAreaLevelCombobox();

public slots:
    void handlePictureChange(int index);
    void handleChangeAreaLevel(int index);

// Event
protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QList<QOpenGLShaderProgram*> programs;
    GridGeometry *gridGeometry;

    // Textures
    QOpenGLTexture *srcTexture;
    QOpenGLTexture *areaTexture;

    // Buffers
    QOpenGLFramebufferObject *edgeRT;
    QOpenGLFramebufferObject *blendWeightRT;
    QOpenGLFramebufferObject *neighborhoodBlendingRT;

    // Output Color Attachment
    QVector<GLuint> bufferTextures;
    QOpenGLTexture *blendWeightBufferTexture;
    QOpenGLTexture *blendWeightAlphaBufferTexture;

private:
    QComboBox *myCombobox;
    QComboBox *myAreaLevel;
    QMap<int, int> indexMapArea;
    int myLevel = 0;
    bool simpleDemo = false;

public slots:
    void cleanup();
};


#endif //INHOUSE_QTOPENGL_GLWIDGET_H
