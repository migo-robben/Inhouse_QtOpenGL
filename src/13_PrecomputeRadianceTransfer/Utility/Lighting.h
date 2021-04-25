#ifndef INHOUSE_QTOPENGL_PRT_LIGHTING_H
#define INHOUSE_QTOPENGL_PRT_LIGHTING_H


#include <QString>
#include <QtMath>
#include <QList>
#include <QFile>
#include <QDataStream>
#include <QVector2D>
#include <QVector3D>
#include <QDebug>

enum LightType {
    PROBE = 0,
    CROSS,
};

class Lighting {
public:
    Lighting() = default;
    ~Lighting();

    Lighting(const QString& path, LightType type=PROBE, int band=3);

    void processingData(int numbersOfSampler, bool useTexture);
    QVector3D probeColor(QVector3D dir);

    void saveToDisk(const QString& outFile);
    void readFromDisk(const QString& inFile);

public:
    QVector<QVector3D> coefficient{};
    QString fileName;

private:
    int band{};

    QString path;
    LightType type;

    int width{};
    int height{};
    int channels{};
    float *data{};
};


#endif
