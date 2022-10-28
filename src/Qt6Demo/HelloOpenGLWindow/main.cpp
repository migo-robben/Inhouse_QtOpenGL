#include <QtGui>
#include "hellowindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    HelloWindow window;
#ifndef __EMSCRIPTEN__
    window.show();
#else
    window.showFullScreen();
#endif

    return QGuiApplication::exec();
}