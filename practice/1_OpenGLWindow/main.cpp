#include <QApplication>

#include "PTriangleWindow.h"

int main(int argc, char **argv){

    QApplication app(argc, argv);
    QSurfaceFormat format;
    format.setSamples(16);

    PTriangleWindow w;
    w.resize(520, 520);
    w.setFormat(format);
    w.setMinimumSize(QSize(100, 100));
    w.show();
    w.setAnimating(true);

    return QApplication::exec();
}