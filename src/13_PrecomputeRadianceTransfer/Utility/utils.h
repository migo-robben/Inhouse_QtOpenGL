#ifndef INHOUSE_QTOPENGL_PRT_UTILS_H
#define INHOUSE_QTOPENGL_PRT_UTILS_H

#include <QVector3D>
#include <QDebug>

#define M_ZERO 1e-9
#define M_DELTA 1e-6f

struct Triangle {
    QVector3D _v0, _v1, _v2; // for triangle vertex
    int _index;

    Triangle() {
        _index = -1;
    }

    Triangle(QVector3D v0, QVector3D v1, QVector3D v2, int index)
            : _v0(v0),
              _v1(v1),
              _v2(v2),
              _index(index) {}
};

struct Ray {
    // original point
    QVector3D _o;
    // direction
    QVector3D _dir;
    QVector3D _inv;
    // point at parameter
    double _tmin;
    double _tmax;
    double _t;
    int _index;

    Ray() = default;
    Ray(QVector3D o, QVector3D d, double tmin = M_DELTA, double tmax = DBL_MAX)
            : _o(o), _dir(d), _tmin(tmin), _tmax(tmax) {
        _inv.setX(1.0/_dir.x());
        _inv.setY(1.0/_dir.y());
        _inv.setZ(1.0/_dir.z());

        _t = DBL_MAX;

        _dir = _dir.normalized();
    }
};

inline float Trimax(float a, float b, float c) {
    return std::max(std::max(a, b), std::max(a, c));
}

inline float Trimin(float a, float b, float c) {
    return std::min(std::min(a, b), std::min(a, c));
}

inline bool rayTriangle(Ray& ray, Triangle& in) {
    // Moller–Trumbore intersection algorithm
    QVector3D E1 = in._v1 - in._v0;
    QVector3D E2 = in._v2 - in._v0;
    QVector3D S = ray._o - in._v0;
    QVector3D S1 = QVector3D::crossProduct(ray._dir, E2);
    QVector3D S2 = QVector3D::crossProduct(S, E1);

    float S1DotE1 = QVector3D::dotProduct(S1, E1);
    if (S1DotE1 < M_ZERO)
        return false;

    float beta = 1.0f / S1DotE1 * QVector3D::dotProduct(S1, S);
    if (beta < 0.0f || beta > 1.0f)
        return false;

    float gamma = 1.0f / S1DotE1 * QVector3D::dotProduct(S2, ray._dir);
    if (gamma < 0.0f || gamma + beta > 1.0f)
        return false;

    float t = 1.0f / S1DotE1 * QVector3D::dotProduct(S2, E2);
    if (t >= ray._tmin && t <= ray._tmax) {
        ray._t = t;
        ray._index = in._index;
        return true;
    }

    return false;
}

inline double factorial(int n) {
    if (n <= 1)
        return 1.0;

    double result = 1.0;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }

    return result;
}

inline double doubleFactorial(int n) {
    if (n <= 1)
        return 1.0;

    double result = 1.0;
    for (unsigned i = n; i > 1; i -= 2) {
        result *= i;
    }

    return result;
}

inline void barycentric(QVector3D &pc, QVector<QVector3D> &triangleVertices, float &u, float &v, float &w) {
    QVector3D v0 = triangleVertices[1] - triangleVertices[0];
    QVector3D v1 = triangleVertices[2] - triangleVertices[0];
    QVector3D v2 = pc - triangleVertices[0];

    float d00 = QVector3D::dotProduct(v0, v0);
    float d01 = QVector3D::dotProduct(v0, v1);
    float d11 = QVector3D::dotProduct(v1, v1);
    float d20 = QVector3D::dotProduct(v2, v0);
    float d21 = QVector3D::dotProduct(v2, v1);
    float denom = d00 * d11 - d01 * d01;

    if (fabs(denom) < M_ZERO) {
        u = v = w = 1.0f / 3.0f;
        return ;
    }

    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;
}

#endif
