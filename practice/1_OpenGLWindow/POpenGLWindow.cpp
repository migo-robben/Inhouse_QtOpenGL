#include "POpenGLWindow.h"

POpenGLWindow::POpenGLWindow(){
    setSurfaceType(QOpenGLWindow::OpenGLSurface);
}

POpenGLWindow::~POpenGLWindow(){

}

void POpenGLWindow::render() {
    if (!m_device)
        m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_device->setSize(size() * devicePixelRatio());
    m_device->setDevicePixelRatio(devicePixelRatio());

    QPainter painter(m_device);
    render(&painter);
}

void POpenGLWindow::exposeEvent(QExposeEvent *event) {
    QWindow::exposeEvent(event);
    qDebug() << "exposed: " << isExposed();
    Q_UNUSED(event)

    if(isExposed()){
        renderNow();
    }
}

void POpenGLWindow::renderNow() {
    if(!isExposed())
        return;
    bool needsInitialize = false;

    if(!m_context){
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();
        needsInitialize = true;
    }
    m_context->makeCurrent(this);
    if(needsInitialize){
        initializeOpenGLFunctions();
        initialize();
    }
    render();
    m_context->swapBuffers(this);

    if(m_animating)
        renderLater();
}

void POpenGLWindow::initialize() {

}

bool POpenGLWindow::event(QEvent *event) {
    qDebug() << "event: " << event->type();
    switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            return true;
        default:
            return QWindow::event(event);
    }
}

void POpenGLWindow::render(QPainter *painter) {
    Q_UNUSED(painter);

    QPainterPath pathPainter;
    pathPainter.setFillRule(Qt::WindingFill);
    pathPainter.addRoundedRect(50,50,100,100,5,5);
    painter->setPen(QPen(Qt::NoPen));
    painter->setBrush(QBrush(QColor("#43FCA2")));
    painter->drawPath(pathPainter.simplified());
}

void POpenGLWindow::renderLater() {
    requestUpdate();
}

void POpenGLWindow::setAnimating(bool animating) {
    m_animating = animating;
    if(animating)
        renderLater();
}
