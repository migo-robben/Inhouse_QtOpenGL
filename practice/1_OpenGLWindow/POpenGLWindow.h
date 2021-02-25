#pragma once
#include <QOpenGLWindow>
#include <QDebug>
#include <QOpenGLFunctions_4_5_Core>

class POpenGLWindow: public QOpenGLWindow, protected QOpenGLFunctions_4_5_Core{
public:
    explicit POpenGLWindow();
    ~POpenGLWindow();

    virtual void render();

public slots:
    void renderNow();

protected:
    void exposeEvent(QExposeEvent* event) override;

};