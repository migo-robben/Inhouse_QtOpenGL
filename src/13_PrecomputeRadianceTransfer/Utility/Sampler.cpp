#include "Sampler.h"
#include "SphericalHarmonics.h"

#include <random>

Sample::Sample(QVector3D cartesianCoord, QVector2D sphericalCoord)
        : cartesianCoord(cartesianCoord), sphericalCoord(sphericalCoord) {
}

Sampler::Sampler(unsigned int n) {
    static QRandomGenerator *randomEngine = QRandomGenerator::global();
    static std::uniform_real_distribution<float> random(0, 1);

    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            QVector2D spherical;
            QVector3D cartesian;

            float x = ((float)i + random(*randomEngine)) / (float)n;
            float y = ((float)j + random(*randomEngine)) / (float)n;

            spherical.setX(2.0 * qAcos(qSqrt(1 - x))); // theta
            spherical.setY(2.0 * M_PI * y); // phi

            cartesian.setX(qSin(spherical.x()) * qCos(spherical.y()));
            cartesian.setY(qSin(spherical.x()) * qSin(spherical.y()));
            cartesian.setZ(qCos(spherical.x()));

            samples << Sample(cartesian, spherical);
        }
    }
}

void Sampler::computeSH(int band) {
    int bandPower2 = band * band;
    int numbersOfSamples = samples.size();

    for (int i = 0; i < numbersOfSamples; i++) {
        samples[i].SHValue = new float[bandPower2];
        for (int l = 0; l < band; l++) {
            for (int m = -l; m <= l; m++) {
                int index = l * (l + 1) + m;
                samples[i].SHValue[index] = (float)SphericalH::SphericalHarmonic(
                        samples[i].sphericalCoord.x(),
                        samples[i].sphericalCoord.y(),
                        l, m);
            }
        }
    }
}
