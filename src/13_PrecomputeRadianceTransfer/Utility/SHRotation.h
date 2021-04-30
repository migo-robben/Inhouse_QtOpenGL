#ifndef INHOUSE_QTOPENGL_PRT_SHROTATION_H
#define INHOUSE_QTOPENGL_PRT_SHROTATION_H


#include "Lighting.h"
#include "DiffuseObject.h"

#include <Eigen/Dense>

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

    static QVector<QVector3D> FastSHRotate(
            unsigned int band,
            QMatrix4x4 rotateMatrix,
            QVector<QVector3D> &inLightCoefficient);

    static void rotate_X(QVector<QVector3D> &out, unsigned int band, float a, QVector<QVector3D> &inLightCoefficient);
    static void SHRotateZ(QVector<QVector3D> &out, unsigned int band, float angle, QVector<QVector3D> &inLightCoefficient);

    static Eigen::Matrix3d computeSquareMatrix_3by3(Eigen::Matrix4f rotationMatrix);
    static Eigen::MatrixXd computeSquareMatrix_5by5(Eigen::Matrix4f rotationMatrix);

    static Eigen::Matrix4f qtMatrix2EigenMatrix(QMatrix4x4 qtRotationMatrix);
};


#endif
