#ifndef INHOUSE_QTOPENGL_PRT_BOUNDINGBOX_H
#define INHOUSE_QTOPENGL_PRT_BOUNDINGBOX_H


#include "utils.h"
#include <QVector3D>

class BBox {
public:
    BBox() = default;
    BBox(QVector3D pMin, QVector3D pMax);
    explicit BBox(Triangle& in);
    explicit BBox(QVector<Triangle>& inlist);

    bool rayIntersect(Ray& r);

    QVector<QVector3D> _v; // represent min and max point with current bounding box, 2 size
    QVector3D _center;

private:
    void setCenter() {
        _center = (_v[0] + _v[1]) / 2.0;
    }
};

inline BBox merge(BBox b1, BBox b2) {
    QVector3D pMin, pMax;
    pMin.setX(std::min(b1._v[0].x(), b2._v[0].x()));
    pMin.setY(std::min(b1._v[0].y(), b2._v[0].y()));
    pMin.setZ(std::min(b1._v[0].z(), b2._v[0].z()));

    pMax.setX(std::max(b1._v[1].x(), b2._v[1].x()));
    pMax.setY(std::max(b1._v[1].y(), b2._v[1].y()));
    pMax.setZ(std::max(b1._v[1].z(), b2._v[1].z()));

    return BBox(pMin, pMax);
}


#endif
