#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

#include "GLWidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("StencilTest");

    GLWidget w;
    w.show();

    return QApplication::exec();
}