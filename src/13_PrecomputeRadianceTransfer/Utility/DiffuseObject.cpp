#include "DiffuseObject.h"

void DiffuseObject::processingData(int mode, int band, int numbersOfSampler, int bounce) {
    _band = band;

    int sqrtNumbersOfSampler = (int)qSqrt(numbersOfSampler);
    int numbersOfVertices = getVerticesData().length();
    int bandPower2 = band * band;

    Sampler sphereSampler(sqrtNumbersOfSampler);
    sphereSampler.computeSH(band);

    BVHTree bvht;
    if (mode == 1) {
        // Transfer type : unshadow
        diffuseUnshadow(numbersOfVertices, bandPower2, &sphereSampler, T_UNSHADOW);
    }
    else if (mode == 2) {
        // Transfer type : shadowed
        diffuseShadow(numbersOfVertices, bandPower2, &sphereSampler, T_SHADOW, bvht);
    }
    else if (mode == 3) {
        // Transfer type : interreflect
        diffuseInterreflect(numbersOfVertices, bandPower2, &sphereSampler, T_INTERREFLECT, bvht, bounce);
    }
}

void DiffuseObject::diffuseUnshadow(int numbersOfVertices, int band2, Sampler *sampler, TransferType type) {
    QVector<QVector3D> empty;
    empty.resize(band2);

    _TransferFunc.clear();
    _TransferFunc.resize(numbersOfVertices);
    _TransferFunc.fill(empty);

    // Sample
    const int numbersOfSampler = sampler->samples.size();

#pragma omp parallel for
    for (int i = 0; i < numbersOfVertices; i++) {
        QVector3D normal = vertices[i].normal;
        for (int j = 0; j < numbersOfSampler; j++) {
            QVector3D sample_cartesianCoord = sampler->samples[j].cartesianCoord;
            float cosineTerm = qMax(QVector3D::dotProduct(normal, sample_cartesianCoord), 0.0f);

            for (int k = 0; k < band2; k++) {
                float SHValue = sampler->samples[j].SHValue[k];
                _TransferFunc[i][k] += _albedo * SHValue * cosineTerm;
            }
        }
    }

    // Normalization.
    float weight = 4.0 * M_PI / numbersOfSampler;
#pragma omp parallel for
    for (int i = 0; i < numbersOfVertices; i++) {
        for (int j = 0; j < band2; j++) {
            _TransferFunc[i][j] *= weight;
        }
    }
}

void DiffuseObject::diffuseShadow(int numbersOfVertices, int band2, Sampler *sampler, TransferType type, BVHTree &bvht) {
}

void DiffuseObject::diffuseInterreflect(int numbersOfVertices, int band2, Sampler *sampler, TransferType type, BVHTree &bvht, int numbersOfBounce) {
}

void DiffuseObject::saveToDisk(const QString &outFile) {
    int bandPower2 = _band * _band;
    int numbersOfVertices = getVerticesData().length();

    QFile file(outFile);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    out << qint32(numbersOfVertices);
    out << qint32(_band);

    for (int i = 0; i < numbersOfVertices; i++) {
        for (int j = 0; j < bandPower2; j++) {
            out << _TransferFunc[i][j];
        }
    }

    file.close();
}

void DiffuseObject::readFromDisk(const QString &inFile) {
    QFile file(inFile);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    qint32 numbersOfVertices;
    in >> numbersOfVertices;

    qint32 band;
    in >> band;

    qint32 bandPower2 = band * band;

    QVector<QVector3D> empty;
    empty.resize(bandPower2);

    _TransferFunc.clear();
    _TransferFunc.resize(numbersOfVertices);
    _TransferFunc.fill(empty);

    for (int i = 0; i < numbersOfVertices; i++) {
        for (int j = 0; j < bandPower2; j++) {
            QVector3D tempData;
            in >> tempData;

            _TransferFunc[i][j] = tempData;
        }
    }

    file.close();
}
