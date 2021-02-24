#ifndef _QTOPENGLREFERENCE_INHOUSE_OPENGLWINDOW_H
#define _QTOPENGLREFERENCE_INHOUSE_OPENGLWINDOW_H

#include <QWindow>
#include <QOpenGLFunctions_4_5_Core>

class OpenGLWindow : public QWindow, protected QOpenGLFunctions_4_5_Core {
public:
    explicit OpenGLWindow(QWindow *parent = nullptr);
    ~OpenGLWindow() override;

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

protected:
    bool event(QEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

public slots:
    void renderNow();

private:
    QOpenGLContext *gl_context;
    QOpenGLPaintDevice *gl_device;
};


#endif
