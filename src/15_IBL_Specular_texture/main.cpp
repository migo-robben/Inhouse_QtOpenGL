#include <QSurfaceFormat>
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QTime>

#include "mainwidget.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("IBL Specular Texture");

    MainWidget w;
    w.setGeometry(640, 140, w.sizeHint().width(), w.sizeHint().height());
    w.show();

    return QApplication::exec();
}
