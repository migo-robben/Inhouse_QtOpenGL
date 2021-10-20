#include "CaptureWindow.h"

#include <QQuickItem>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QBuffer>
#include <QDesktopWidget>

#include <iostream>

CaptureWindow::CaptureWindow(QWidget *parent) {
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setMinimumHeight(680);
    this->setMinimumWidth(680);
    initUI();
}

void CaptureWindow::initUI() {
    vboxlayout = new QVBoxLayout;
    vboxlayout->setSpacing(0);
    vboxlayout->setContentsMargins(0, 0, 0, 0);
    
    // Init widget
    painterWidget = new QQuickWidget;
    painterWidget->setAttribute(Qt::WA_TranslucentBackground);
    painterWidget->setClearColor(Qt::transparent);
    painterWidget->setSource(QUrl::fromLocalFile("src/25_QMLScreenshotExample/QML/painterWidget.qml"));

    // layout setting
    this->setLayout(vboxlayout);

    // add widget within layout
    vboxlayout->addWidget(painterWidget);
}

QSize CaptureWindow::sizeHint() const {
    return { 1000, 580 };
}

QVector<QQmlEngine *> CaptureWindow::getQmlEngine() {
    if (!engines.isEmpty()) {
        engines.clear();
    }

    engines << painterWidget->engine();

    return engines;
}

void CaptureWindow::resizeEvent(QResizeEvent *event) {
    QMetaObject::invokeMethod(
            painterWidget->rootObject(),
            "resizeWidthHeight",
            Q_ARG(QVariant, width()),
            Q_ARG(QVariant, height()));
}

void CaptureWindow::captureScreen(const QRect &geo, int offsetY, int biasHeight) {
    QList<QScreen *> screens = QApplication::screens();
    QList<QPixmap> screens_pixmap;
    int w = 0, h = 0, p = 0;

    for (auto screen : screens) {
        QRect g = screen->geometry();
        QPixmap pix = screen->grabWindow(
                QApplication::desktop()->winId(),
                g.x(), g.y(), g.width(), g.height());
        w += pix.width();
        h = std::max(h, pix.height());

        screens_pixmap << pix;
    }

    QPixmap final(w, h);
    QPainter painter(&final);
    final.fill(Qt::black);

    for (const auto& scr : screens_pixmap) {
        painter.drawPixmap(QPoint(p, 0), scr);
        p += scr.width();
    }

    capturePixmap = final.copy(geo.x(), geo.y() + offsetY, geo.width(), geo.height() - offsetY - biasHeight);

    QImage image = capturePixmap.toImage();
    QByteArray byteArray;

    QBuffer buffer(&byteArray);
    image.save(&buffer, "png");
    QString Base64Image = QString::fromLatin1(byteArray.toBase64().data());

    // for html or qml must use "data:image/png;base64" prefix
    // E.g. QString finalBase64Image = QString("data:image/png;base64,") + Base64Image;
    QString finalBase64Image = Base64Image;

    captureImageEncode << finalBase64Image;
}

void CaptureWindow::sendCaptureScreenToQML() {
    if (!captureImageEncode.isEmpty()) {
        QString lastCaptureScreen = QString("data:image/png;base64,") + captureImageEncode.back();
        QMetaObject::invokeMethod(
                painterWidget->rootObject(),
                "receiveCaptureScreenFromCpp",
                Q_ARG(QVariant, lastCaptureScreen));
    }
}
