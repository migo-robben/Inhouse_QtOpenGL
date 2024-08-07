//
// Created by wuguo on 8/5/2024.
//

#ifndef INHOUSE_QTOPENGL_MURRAYRENDERERAPI_H
#define INHOUSE_QTOPENGL_MURRAYRENDERERAPI_H

#include <QObject>
#include <QImage>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

class ContextHandler
{
public:
    ContextHandler();
    ~ContextHandler();

    bool create();
    void makeCurrent();
    void doneCurrent();
    void clearUp();

private:
    QOpenGLContext *context;
    QOffscreenSurface *surface;
};

class MurrayRenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit MurrayRenderer(QObject *parent = nullptr);
    ~MurrayRenderer() override;

    void setWidth(int w) { width = w; }
    void setHeight(int h) { height = h; }

    void initialize();
    void render();
private:
    // frame buffer object
    QOpenGLFramebufferObject *fbo;

    int width;
    int height;
};


#endif //INHOUSE_QTOPENGL_MURRAYRENDERERAPI_H
