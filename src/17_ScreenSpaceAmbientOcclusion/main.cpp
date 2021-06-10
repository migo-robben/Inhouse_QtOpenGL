#include <QSurfaceFormat>
#include <QApplication>
#include <QDebug>

#include "GLWidget.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("SSAO");

    GLWidget w;
    w.setGeometry(640, 140, w.sizeHint().width(), w.sizeHint().height());
    w.show();

    return QApplication::exec();
}
