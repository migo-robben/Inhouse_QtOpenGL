#include "BVHTree.h"

void BVHTree::build(Object &obj) {
    qDebug() << "BVHTree: Start building";

    int numbersOfTriangles = obj.getIndices().count() / 3;
    _triangles.clear();

    QVector<quint64> indices = obj.getIndices();
    QVector<SampleVertexData> vertices = obj.getVerticesData();

    // get each triangle
    for (int i = 0; i < numbersOfTriangles; i++) {
        // for loop every triangle
        int offset = 3 * i; // each triangle start point/vertex

        Triangle triangle;
        QVector<QVector3D> v;
        v.resize(3);
        unsigned int index;

        for (int j = 0; j < 3; j++) {
            index = indices[offset + j];
            v[j] = vertices[(int)index].position;
        }
        _triangles << Triangle(v[0], v[1], v[2], i);
    }
    qDebug() << "Numbers of triangles: " << _triangles.count();

    if (_triangles.count() == 1) {
        _root = new BVHNode(_triangles[0]);
    }
    else if (_triangles.count() == 2) {
        _root = new BVHNode(_triangles[0], _triangles[1]);
    }
    else {
        _root = new BVHNode();
    }

    _root->_bbox = BBox(_triangles);
    QVector3D pivot = _root->_bbox._center;
    int mid_point = split(0, _triangles.count(), pivot[0], XAXIS);

    _root->_left = recursiveBuild(0, mid_point, YAXIS);
    _root->_right = recursiveBuild(mid_point, _triangles.count() - mid_point, YAXIS);

    qDebug() << "BVHTree: Building done";
}

int BVHTree::split(int start, int size, float pivot, int axis) {
    // split two part base from pivot
    BBox b_temp;
    float centroid;
    int index = 0;

    for (int i = start; i < (start + size); i++) {
        b_temp = BBox(_triangles[i]);
        centroid = b_temp._center[axis];

        if (centroid < pivot) {
            Triangle temp = _triangles[i];
            _triangles[i] = _triangles[start + index];
            _triangles[start + index] = temp;
            index++;
        }
    }

    if ((index == 0) || (index == size)) {
        index = size / 2;
    }
    return index;
}

BVHNode *BVHTree::recursiveBuild(int start, int size, int axis) {
    if (size == 1) {
        return new BVHNode(_triangles[start]);
    }
    if (size == 2) {
        return new BVHNode(_triangles[start], _triangles[start+1]);
    }

    BBox b_temp = BBox(_triangles[start]);
    for (int i = start + 1; i < start + size; ++i) {
        b_temp = merge(b_temp, BBox(_triangles[i]));
    }

    QVector3D pivot = b_temp._center;
    int mid_point = split(start, size, pivot[axis], axis);

    BVHNode* result = new BVHNode();

    result->_bbox = b_temp;
    result->_left = recursiveBuild(start, mid_point, (axis + 1) % 3);
    result->_right = recursiveBuild(start + mid_point, size - mid_point, (axis + 1) % 3);

    return result;
}

bool BVHTree::intersect(Ray &ray) {
    return _root->hit(ray);
}
