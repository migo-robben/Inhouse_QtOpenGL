#ifndef INHOUSE_QTOPENGL_PRT_SAMPLER_H
#define INHOUSE_QTOPENGL_PRT_SAMPLER_H


#include <QDebug>
#include <QtMath>
#include <QList>
#include <QVector2D>
#include <QVector3D>
#include <QRandomGenerator>

class Sample {
public:
    Sample(QVector3D cartesianCoord, QVector2D sphericalCoord);
    ~Sample() { delete[] SHValue; }

    QVector3D cartesianCoord{};
    QVector2D sphericalCoord{}; // theta, phi

    float *SHValue{};
};

class Sampler {
public:
    explicit Sampler(unsigned int n);
    void computeSH(int band);

    QList<Sample> samples;
};


#endif
