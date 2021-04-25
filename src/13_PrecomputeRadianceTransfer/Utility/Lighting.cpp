#include "Lighting.h"
#include "utils.h"
#include "Sampler.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Lighting::Lighting(const QString& path, LightType type, int band) : path(path), type(type), band(band) {

    if (path.isEmpty()) {
    }
    else {
        data = stbi_loadf(path.toStdString().c_str(), &width, &height, &channels, 0);
        int beginIndex = path.toStdString().rfind('\\');
        int endIndex = path.toStdString().rfind('.');
        fileName = QString(path.toStdString().substr(beginIndex+1, endIndex-beginIndex-1).c_str());
    }
}

Lighting::~Lighting() {
    delete[] data;
}

QVector3D Lighting::probeColor(QVector3D dir) {
    dir.normalize();
    float d = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());

    float r;
    if (fabs(d) <= M_ZERO) {
        r = 0.0;
    }
    else {
        r = (1.0 / (2.0 * M_PI)) * acos(dir.z()) / d;
    }

    QVector2D texCoord;
    texCoord.setX(0.5 + dir.x() * r);
    texCoord.setY(0.5 + dir.y() * r);

    QVector2D pixelCoord;
    pixelCoord.setX((int)(width * texCoord.x()));
    pixelCoord.setY((int)(height * (1.0 - texCoord.y())));

    int index = pixelCoord.y() * width + pixelCoord.x();
    int offset = 3 * index;

    return QVector3D(data[offset], data[offset+1], data[offset+2]);
}

// compute light function part, include SH basic and coefficient
void Lighting::processingData(int numbersOfSampler, bool useTexture) {
    int sqrtNumbersOfSampler = (int)qSqrt(numbersOfSampler);
    int bandPower2 = band * band;

    // weight is the pdf of monte carlo integration
    float weight = 4.0 * M_PI / numbersOfSampler;

    Sampler sphereSampler(sqrtNumbersOfSampler);
    sphereSampler.computeSH(band);
    coefficient.clear();
    coefficient.resize(bandPower2);

    for (int i = 0; i < numbersOfSampler; i++) {
        QVector3D dir = sphereSampler.samples[i].cartesianCoord;
        for (int j = 0; j < bandPower2; j++) {
            float SHValue = sphereSampler.samples[i].SHValue[j];
            if (useTexture) {
                QVector3D color = probeColor(dir);
                coefficient[j] += color * SHValue;
            }
            else {
                // using simple light probe
            }
        }
    }

    for (int i = 0; i < bandPower2; i++) {
        coefficient[i] = coefficient[i] * weight;
    }
}

void Lighting::saveToDisk(const QString& outFile) {
    int bandPower2 = band * band;

    QFile file(outFile);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    out << qint32(band);
    for (int i=0; i<bandPower2; i++) {
        out << coefficient[i];
    }

    file.close();
}

void Lighting::readFromDisk(const QString& inFile) {
    QFile file(inFile);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    qint32 sampleBand;
    in >> sampleBand;

    coefficient.resize(sampleBand * sampleBand);

    QVector3D sampleCoefficient;
    for (int i=0; i<sampleBand*sampleBand; i++) {
        in >> sampleCoefficient;
        coefficient[i] = sampleCoefficient;
    }

    file.close();
}
