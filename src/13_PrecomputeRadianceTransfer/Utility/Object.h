#ifndef INHOUSE_QTOPENGL_PRT_OBJECT_H
#define INHOUSE_QTOPENGL_PRT_OBJECT_H

#include <QtMath>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QFile>
#include <QDataStream>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

enum TransferType {
    T_UNSHADOW,
    T_SHADOW,
    T_INTERREFLECT
};

struct SampleVertexData
{
    QVector3D position;
    QVector2D texCoord;
    QVector3D normal;
    QVector3D tangent;
    QVector3D bitangent;
};

class Object {
public:
    Object() = default;

    void initGeometry(const QString& modelFilePath, QVector3D albedo, bool calcTangent);
    void processNode(aiNode *node, const aiScene *scene, bool calcTangent);
    void processMesh(aiMesh *mesh, const aiScene *scene, bool calcTangent);

    QVector<SampleVertexData> getVerticesData();
    QVector<quint64> getIndices();

    // Project to SH function.
    virtual void processingData(int mode, int band, int numbersOfSampler, int bounce) = 0;
    virtual void saveToDisk(const QString& outFile) = 0;
    virtual void readFromDisk(const QString& inFile) = 0;

    QString fileName{};

protected:
    QVector<SampleVertexData> vertices;
    QVector<quint64> indices;

    int _band{};
    QVector3D _albedo{};
};


#endif
