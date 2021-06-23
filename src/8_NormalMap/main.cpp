#include <QSurfaceFormat>
#include <QApplication>
#include <QDebug>

#include "mainwidget.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("Load Models");

    MainWidget w;
    w.setMinimumSize(120, 120);
    w.show();

    return QApplication::exec();
}
