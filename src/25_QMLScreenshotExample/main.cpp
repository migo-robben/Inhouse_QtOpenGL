#include <QApplication>

#include <QtQuickWidgets/QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>

#include "CaptureWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto *capWnd = new CaptureWindow;
    QVector<QQmlEngine*> engines = capWnd->getQmlEngine();
    for (auto engine : engines)
        engine->rootContext()->setContextProperty("backend", capWnd);

    capWnd->show();

    return QApplication::exec();
}

#include "main.moc"