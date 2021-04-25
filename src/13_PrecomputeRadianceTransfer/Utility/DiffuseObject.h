#ifndef INHOUSE_QTOPENGL_PRT_DIFFUSEOBJECT_H
#define INHOUSE_QTOPENGL_PRT_DIFFUSEOBJECT_H

#include "Object.h"
#include "Sampler.h"
#include "BVHTree.h"
#include <QDebug>

class DiffuseObject : public Object {
public:
    void processingData(int mode, int band, int numbersOfSampler, int bounce=1) override;
    void saveToDisk(const QString& outFile) override;
    void readFromDisk(const QString& inFile) override;

    QVector<QVector<QVector3D>> _TransferFunc;

private:
    void diffuseUnshadow(int numbersOfVertices, int band2, Sampler *sampler, TransferType type);
    void diffuseShadow(int numbersOfVertices, int band2, Sampler *sampler, TransferType type, BVHTree &bvht);
    void diffuseInterreflect(int numbersOfVertices, int band2, Sampler *sampler, TransferType type, BVHTree &bvht, int numbersOfBounce=1);
};


#endif
