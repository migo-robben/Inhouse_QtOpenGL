//
// Created by PC on 9/2/2024.
//
#include <QApplication>

#include "viewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("OsdViewer");

    Viewer viewer;
    viewer.setFixedSize(640, 640);
    viewer.show();

    return QApplication::exec();
}