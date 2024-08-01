#include <QWidget>
#include <QVBoxLayout>
#include <QtQuickWidgets/QQuickWidget>

#ifndef QTREFERENCE_CAPTUREWINDOW_H
#define QTREFERENCE_CAPTUREWINDOW_H


class CaptureWindow : public QWidget {
    Q_OBJECT
public:
    explicit CaptureWindow(QWidget *parent = nullptr);

    void initUI();
    QSize sizeHint() const override;

    QVector<QQmlEngine*> getQmlEngine();
    void resizeEvent(QResizeEvent *event) override;
    void captureScreen(const QRect &geo, int offsetY, int biasHeight);
    void sendCaptureScreenToQML();

public:
    QVector<QQmlEngine*> engines;
    QPixmap capturePixmap;

protected:
    QVBoxLayout *vboxlayout{};
    QQuickWidget *painterWidget{};
    QList<QString> captureImageEncode;

protected slots:
    void maximizeWindow() {
        this->setWindowState(Qt::WindowMaximized);
    }

    void minimizeWindow() {
        this->setWindowState(Qt::WindowMinimized);
    }

    void normalWindow() {
        this->setWindowState(Qt::WindowNoState);
    }

    void moveWindowPosition(int dx, int dy) {
        this->setGeometry(this->x() + dx, this->y() + dy, width(), height());
    }

    void resizeWindow(int dx, int dy) {
        this->setGeometry(this->x(), this->y(), width() + dx, height() + dy);
    }

    void captureScreenFromQML(int offsetY, int biasHeight) {
        captureScreen(this->geometry(), offsetY, biasHeight);
        sendCaptureScreenToQML();
    }

    void batchSaveCaptureScreenImageToDisk() {
        if (!captureImageEncode.isEmpty()) {
            int imageIndex = 0;
            for (const auto& encodeImage : captureImageEncode) {
                QImage img;
                QByteArray arr_base64 = encodeImage.toLatin1();
                img.loadFromData(QByteArray::fromBase64(arr_base64));

                img.save("src/25_QMLScreenshotExample/CaptureImages/captureImage_" + QString::number(imageIndex++) + ".png");
            }
        }
    }
};

#endif //QTREFERENCE_CAPTUREWINDOW_H
