#include <QDebug>
#include <QApplication>
#include <QSurfaceFormat>
#include "mainwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication::setApplicationName("Plane");

    MainWidget widget;
    widget.show();

    return QApplication::exec();
}