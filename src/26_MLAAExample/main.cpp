#include <QApplication>
#include "GLWidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWidget w;
    w.show();

    return QApplication::exec();
}