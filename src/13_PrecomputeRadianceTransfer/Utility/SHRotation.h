#ifndef INHOUSE_QTOPENGL_PRT_SHROTATION_H
#define INHOUSE_QTOPENGL_PRT_SHROTATION_H


#include "Lighting.h"
#include "DiffuseObject.h"

#include <QVector>
#include <QVector3D>
#include <QMatrix>
#include <QMatrix4x4>
#include <QDebug>

class SHRotation {
public:
    SHRotation() = default;

    static QVector<QVector3D> SHRotate(
            unsigned int band,
            QMatrix4x4 rotateMatrix,
            QVector<QVector3D> &inLightCoefficient);

    static void rotate_X(QVector<QVector3D> &out, unsigned int band, float a, QVector<QVector3D> &inLightCoefficient);
    static void SHRotateZ(QVector<QVector3D> &out, unsigned int band, float angle, QVector<QVector3D> &inLightCoefficient);
};



#endif
