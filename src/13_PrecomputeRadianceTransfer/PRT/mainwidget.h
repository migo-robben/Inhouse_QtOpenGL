#ifndef INHOUSE_QTOPENGL_PRT_UNSHADOW_MAINWIDGET_H
#define INHOUSE_QTOPENGL_PRT_UNSHADOW_MAINWIDGET_H


#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>
#include <QApplication>

#include <QDebug>

#include "Helper/Camera.h"
#include "Helper/CustomGeometry.h"

#include "Utility/Lighting.h"
#include "Utility/DiffuseObject.h"
#include "Utility/SHRotation.h"

class Camera;
class CustomGeometry;

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
Q_OBJECT
public:
    explicit MainWidget(QString lightFunctionData, QString transferFunctionData, QWidget *parent = nullptr);
    ~MainWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool zoomInProcessing = false;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void initShaders();
    void initGeometry();
    void initLightAndTransferFunction(QString lightData, QString TransferData);
    QVector<QVector3D> rotateLightCoefficient();

    void glSetting();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<QOpenGLShaderProgram*> programs;
    CustomGeometry *customGeometry;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    QString lightFuncData;
    QString transferFuncData;

    Lighting lightPattern;
    DiffuseObject diffuseObj;

    int phi;
    bool autoRotateSwitcher = false;

public slots:
    void cleanup();
};


#endif
