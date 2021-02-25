#include <QApplication>

#include "POpenGLWindow.h"

int main(int argc, char **argv){

    QApplication app(argc, argv);

    POpenGLWindow w;
    w.show();

    return QApplication::exec();
}