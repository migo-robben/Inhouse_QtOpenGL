#include "OpenGLWindow.h"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>

OpenGLWindow::OpenGLWindow(QWindow *parent) : QWindow(parent), gl_context(nullptr), gl_device(nullptr) {
    setSurfaceType(QWindow::OpenGLSurface);
}

OpenGLWindow::~OpenGLWindow() {
    delete gl_device;
    delete gl_context;
}

void OpenGLWindow::initialize() {
}

void OpenGLWindow::render(QPainter *painter) {
    QPainterPath pathPainter;
    pathPainter.setFillRule(Qt::WindingFill);
    pathPainter.addRoundedRect(50,50,100,100,5,5);
    painter->setPen(QPen(Qt::NoPen));
    painter->setBrush(QBrush(QColor("#43FCA2")));
    painter->drawPath(pathPainter.simplified());
}

void OpenGLWindow::render() {
    if (!gl_device)
        gl_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    gl_device->setSize(size() * devicePixelRatio());
    gl_device->setDevicePixelRatio(devicePixelRatio());

    QPainter painter(gl_device);
    render(&painter);
}

bool OpenGLWindow::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            return true;
        default:
            return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event) {
    Q_UNUSED(event);

    if (isExposed()) {
        qDebug() << "Show Window";
        renderNow();
    }
}

void OpenGLWindow::renderNow() {
    if (!isExposed())
        return ;

    bool needsInitialize = false;

    if (!gl_context) {
        gl_context = new QOpenGLContext(this);
        gl_context->setFormat(requestedFormat());
        gl_context->create();

        needsInitialize = true;
    }

    gl_context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();
    gl_context->swapBuffers(this); // render 之后才swapBuffers
}