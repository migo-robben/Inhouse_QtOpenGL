//
// Created by wuguo on 8/5/2024.
//
#include <vector>

#include <QApplication>
#include <QImage>

#include "MurrayRenderer.h"


void copyImageToVector() {
    QImage image = QImage("F:/PycharmProjects/PythonKnowladge/USD/usdview/RenderOut/MurrayRenderer.png");
    image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const uint8_t* pixelData = image.bits();
    int dataSize = image.sizeInBytes();

    std::vector<uint8_t> _buffer(pixelData, pixelData + dataSize);

    qDebug() << "-----> " << image.format() << image.size() << dataSize << pixelData[0];
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ContextHandler contextHandler;
    if (!contextHandler.create())
        return -1;

    MurrayRenderer murrayRenderer;

    // 在 OpenGL 上下文中渲染
    contextHandler.makeCurrent();
    murrayRenderer.initialize();
    murrayRenderer.render();
    contextHandler.doneCurrent();

    return QApplication::exec();
}
