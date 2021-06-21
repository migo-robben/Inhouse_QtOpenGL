#include <QSurfaceFormat>
#include <QApplication>
#include <QDebug>
#include <QTime>

#include "GLWidget.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("The Kullay-Conty Approximation");

    GLWidget w;
    w.show();

    return QApplication::exec();
}
