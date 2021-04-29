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
    QVector<QVector3D> empty;
    empty.resize(band2);

    _TransferFunc.clear();
    _TransferFunc.resize(numbersOfVertices);
    _TransferFunc.fill(empty);

    // Sample
    const int numbersOfSampler = sampler->samples.size();

    // Build BVH Tree
    if (type == T_SHADOW)
        bvht.build(*this);

    bool visibility = true;
#pragma omp parallel for
    for (int i = 0; i < numbersOfVertices; i++) {
        QVector3D normal = vertices[i].normal;
        for (int j = 0; j < numbersOfSampler; j++) {
            QVector3D sample_cartesianCoord = sampler->samples[j].cartesianCoord;
            float cosineTerm = QVector3D::dotProduct(normal, sample_cartesianCoord);

            if (cosineTerm > 0.0) {
                // Ray test
                Ray testRay(vertices[i].position, sample_cartesianCoord);
                visibility = !bvht.intersect(testRay);

                if (!visibility)
                    cosineTerm = 0.0;
            }
            else {
                cosineTerm = 0.0;
            }

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

void DiffuseObject::diffuseInterreflect(int numbersOfVertices, int band2, Sampler *sampler, TransferType type, BVHTree &bvht, int numbersOfBounce) {
    bvht.build(*this);

    diffuseShadow(numbersOfVertices, band2, sampler, type, bvht);

    // Sample
    const int numbersOfSampler = sampler->samples.size();

    auto interReflect = new QVector<QVector<QVector3D>>[numbersOfBounce + 1];
    interReflect[0] = _TransferFunc;

    QVector<QVector3D> empty;
    empty.resize(band2);

    float weight = 4.0 * M_PI / numbersOfSampler;

    for (int k = 0; k < numbersOfBounce; k++) {
        QVector<QVector<QVector3D>> zeroVector;
        zeroVector.resize(numbersOfVertices);
        zeroVector.fill(empty);

        interReflect[k+1].resize(numbersOfVertices);

#pragma omp parallel for
        for (int i = 0; i < numbersOfVertices; i++) {
            QVector3D normal = vertices[i].normal;
            for (int j = 0; j < numbersOfSampler; j++) {
                QVector3D sample_cartesianCoord = sampler->samples[j].cartesianCoord;
                float cosineTerm = QVector3D::dotProduct(normal, sample_cartesianCoord);

                if (cosineTerm > 0.0) {
                    // Ray test
                    Ray testRay(vertices[i].position, sample_cartesianCoord);
                    bool visibility = !bvht.intersect(testRay);

                    if (visibility)
                        continue;

                    int hitTriangleIndex = testRay._index;
                    int hitTriangleStartIndicesIndex = hitTriangleIndex * 3;

                    QVector<QVector3D> hittedTriangleVertices;
                    QVector<QVector<QVector3D>> SHTransferFunc;

                    for (int m = 0; m < 3; m++) {
                        unsigned int index = indices[hitTriangleStartIndicesIndex + m];
                        hittedTriangleVertices << vertices[(int)index].position;
                        SHTransferFunc << interReflect[k].at((int)index);
                    }
                    float u, v, w;
                    QVector3D pc = testRay._o + (float)testRay._t * testRay._dir; // o + td;
                    barycentric(pc, hittedTriangleVertices, u, v, w);

                    QVector<QVector3D> SHtemp;
                    SHtemp.resize(band2);
                    for (int m = 0; m < band2; m++) {
                        SHtemp[m] = u * SHTransferFunc[0].at(m) + v * SHTransferFunc[1].at(m) + w * SHTransferFunc[2].at(m);
                        zeroVector[i][m] += _albedo * SHtemp[m] * cosineTerm;
                    }
                }
            }
        }

#pragma omp parallel for
        for (int i = 0; i < numbersOfVertices; i++) {
            interReflect[k + 1][i].resize(band2);
            for (int j = 0; j < band2; j++) {
                zeroVector[i][j] *= weight;
                interReflect[k + 1][i][j] = interReflect[k].at(i).at(j) + zeroVector.at(i).at(j);
            }
        }
    }

    _TransferFunc = interReflect[numbersOfBounce];
    delete[] interReflect;
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
