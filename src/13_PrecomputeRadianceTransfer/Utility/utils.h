#ifndef INHOUSE_QTOPENGL_PRT_UTILS_H
#define INHOUSE_QTOPENGL_PRT_UTILS_H

#include <QVector3D>
#include <QDebug>

#define M_ZERO 1e-9
#define M_DELTA 1e-6f

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

#endif
