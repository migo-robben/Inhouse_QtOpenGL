#ifndef QTREFERENCE_READUSD_GLWIDGET_H
#define QTREFERENCE_READUSD_GLWIDGET_H

#include <QApplication>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLFramebufferObjectFormat>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

#include "Helper/Camera.h"
#include "usdParser.h"

#include <map>

class Camera;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool zoomInProcessing = false;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void initShaders();
    void initGeometry();
    void initTexture();

    void glSetting();
    void parseUSDFile(QString &usdFilePath);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override ;
    void wheelEvent(QWheelEvent *event) override;

private:
    QVector<QOpenGLShaderProgram*> programs;

    Camera *camera;
    QMatrix4x4 model;
    QPoint mousePos;

    int drewNumber = 0;
    int lastDrew = 0;
    QElapsedTimer drewTimer;

    std::shared_ptr<usdParser> parser;

public slots:
    void cleanup();
};

#endif
