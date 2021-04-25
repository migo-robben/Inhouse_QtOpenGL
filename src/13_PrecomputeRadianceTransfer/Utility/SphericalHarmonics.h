#ifndef INHOUSE_QTOPENGL_PRT_SPHERICALHARMONICS_H
#define INHOUSE_QTOPENGL_PRT_SPHERICALHARMONICS_H

#include "utils.h"
#include <QtMath>

namespace SphericalH {
    // Normalization constants.
    double static Kvalue(int l, int m) {
        if (m == 0) {
            return qSqrt((2 * l + 1) / (4 * M_PI));
        }

        double up = (2 * l + 1) * factorial(l - abs(m));
        double down = (4 * M_PI) * factorial(l + abs(m));

        return qSqrt(up / down);
    }

    double static Legendre(double x, int l, int m) {
        double result = 0.0;
        if (l == m) {
            result = pow(-1, m) * doubleFactorial(2 * m - 1) * pow((1 - x * x), m / 2.0);
        }
        else if (l == m + 1) {
            result = x * (2 * m + 1) * Legendre(x, m, m);
        }
        else {
            result = (x * (2 * l - 1) * Legendre(x, l - 1, m) - (l + m - 1) * Legendre(x, l - 2, m)) / (l - m);
        }

        return result;
    }

    // Value for Spherical Harmonic.
    double static SphericalHarmonic(double theta, double phi, int l, int m) {
        double result = 0.0;
        if (m == 0) {
            result = Kvalue(l, 0) * Legendre(cos(theta), l, 0);
        }
        else if (m > 0) {
            result = qSqrt(2.0f) * Kvalue(l, m) * cos(m * phi) * Legendre(cos(theta), l, m);
        }
        else {
            result = qSqrt(2.0f) * Kvalue(l, m) * sin(-m * phi) * Legendre(cos(theta), l, -m);
        }

        if (fabs(result) <= M_ZERO)
            result = 0.0;

        return result;
    }
}

#endif
