#pragma once
#include <QOpenGLWindow>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QScreen>
#include <QOpenGLFunctions_4_5_Core>

class POpenGLWindow: public QWindow, protected QOpenGLFunctions_4_5_Core{
public:
    explicit POpenGLWindow();
    ~POpenGLWindow();

    virtual void initialize();
    virtual void render();
    virtual void render(QPainter *painter);
    void setAnimating(bool animating);

public slots:
    void renderNow();
    void renderLater();

protected:
    bool event(QEvent* event) override;
    void exposeEvent(QExposeEvent* event) override;

private:
    QOpenGLContext* m_context = nullptr;
    QOpenGLPaintDevice* m_device = nullptr;
    bool m_animating;
};