#include "POpenGLWindow.h"

POpenGLWindow::POpenGLWindow(){
    setSurfaceType(QWindow::OpenGLSurface);
}

POpenGLWindow::~POpenGLWindow(){

}

void POpenGLWindow::render() {

}

void POpenGLWindow::exposeEvent(QExposeEvent *event) {
    QPaintDeviceWindow::exposeEvent(event);
    qDebug() << "exposed: " << isExposed();
    Q_UNUSED(event)

    if(isExposed()){
        renderNow();
    }
}
