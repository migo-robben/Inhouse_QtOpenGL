#include "SHRotation.h"

#define SH_MAXORDER 6
#define SH_MINORDER 2

QVector<QVector3D> SHRotation::SHRotate(
        unsigned int band,
        QMatrix4x4 rotateMatrix,
        QVector<QVector3D> &inLightCoefficient)
{
    QMatrix3x3 n_rotateMatrix = rotateMatrix.normalMatrix();

    float alpha, beta, gamma, sinb;
    int bandPower2 = band * band;

    QVector<QVector3D> temp, temp1, outLightCoefficient;
    outLightCoefficient.resize(bandPower2);
    temp.resize(bandPower2);
    temp1.resize(bandPower2);

    // First step, set band 0
    outLightCoefficient[0] = inLightCoefficient[0];

    if (band > SH_MAXORDER || band < SH_MINORDER) {
        return outLightCoefficient;
    }

    if (band <= 3) {
        outLightCoefficient[1] = n_rotateMatrix.data()[4] * inLightCoefficient[1] - n_rotateMatrix.data()[7] * inLightCoefficient[2] + n_rotateMatrix.data()[1] * inLightCoefficient[3];
        outLightCoefficient[2] = n_rotateMatrix.data()[5] * inLightCoefficient[1] + n_rotateMatrix.data()[8] * inLightCoefficient[2] - n_rotateMatrix.data()[2] * inLightCoefficient[3];
        outLightCoefficient[3] = n_rotateMatrix.data()[3] * inLightCoefficient[1] - n_rotateMatrix.data()[6] * inLightCoefficient[2] + n_rotateMatrix.data()[0] * inLightCoefficient[3];

        if (band == 3) {
            float coeff[] = {
                    n_rotateMatrix.data()[3] * n_rotateMatrix.data()[0], n_rotateMatrix.data()[4] * n_rotateMatrix.data()[1],
                    n_rotateMatrix.data()[4] * n_rotateMatrix.data()[7], n_rotateMatrix.data()[3] * n_rotateMatrix.data()[6],
                    n_rotateMatrix.data()[6] * n_rotateMatrix.data()[6], n_rotateMatrix.data()[7] * n_rotateMatrix.data()[7],
                    n_rotateMatrix.data()[0] * n_rotateMatrix.data()[6], n_rotateMatrix.data()[1] * n_rotateMatrix.data()[7],
                    n_rotateMatrix.data()[1] * n_rotateMatrix.data()[1], n_rotateMatrix.data()[3] * n_rotateMatrix.data()[3],
                    n_rotateMatrix.data()[4] * n_rotateMatrix.data()[4], n_rotateMatrix.data()[0] * n_rotateMatrix.data()[0], };

            outLightCoefficient[4] = (n_rotateMatrix.data()[4] * n_rotateMatrix.data()[0] + n_rotateMatrix.data()[1] * n_rotateMatrix.data()[3]) * inLightCoefficient[4];
            outLightCoefficient[4] -= (n_rotateMatrix.data()[3] * n_rotateMatrix.data()[7] + n_rotateMatrix.data()[4] * n_rotateMatrix.data()[6]) * inLightCoefficient[5];
            outLightCoefficient[4] += 1.7320508076f * n_rotateMatrix.data()[6] * n_rotateMatrix.data()[7] * inLightCoefficient[6];
            outLightCoefficient[4] -= (n_rotateMatrix.data()[1] * n_rotateMatrix.data()[6] + n_rotateMatrix.data()[0] * n_rotateMatrix.data()[7]) * inLightCoefficient[7];
            outLightCoefficient[4] += (n_rotateMatrix.data()[0] * n_rotateMatrix.data()[1] - n_rotateMatrix.data()[3] * n_rotateMatrix.data()[4]) * inLightCoefficient[8];

            outLightCoefficient[5] = (n_rotateMatrix.data()[4] * n_rotateMatrix.data()[8] + n_rotateMatrix.data()[5] * n_rotateMatrix.data()[7]) * inLightCoefficient[5];
            outLightCoefficient[5] -= (n_rotateMatrix.data()[4] * n_rotateMatrix.data()[2] + n_rotateMatrix.data()[5] * n_rotateMatrix.data()[1]) * inLightCoefficient[4];
            outLightCoefficient[5] -= 1.7320508076f * n_rotateMatrix.data()[8] * n_rotateMatrix.data()[7] * inLightCoefficient[6];
            outLightCoefficient[5] += (n_rotateMatrix.data()[2] * n_rotateMatrix.data()[7] + n_rotateMatrix.data()[1] * n_rotateMatrix.data()[8]) * inLightCoefficient[7];
            outLightCoefficient[5] -= (n_rotateMatrix.data()[1] * n_rotateMatrix.data()[2] - n_rotateMatrix.data()[4] * n_rotateMatrix.data()[5]) * inLightCoefficient[8];

            outLightCoefficient[6] = (n_rotateMatrix.data()[8] * n_rotateMatrix.data()[8] - 0.5f * (coeff[4] + coeff[5])) * inLightCoefficient[6];
            outLightCoefficient[6] -= (0.5773502692f * (coeff[0] + coeff[1]) - 1.1547005384f * n_rotateMatrix.data()[5] * n_rotateMatrix.data()[2]) * inLightCoefficient[4];
            outLightCoefficient[6] += (0.5773502692f * (coeff[2] + coeff[3]) - 1.1547005384f * n_rotateMatrix.data()[5] * n_rotateMatrix.data()[8]) * inLightCoefficient[5];
            outLightCoefficient[6] += (0.5773502692f * (coeff[6] + coeff[7]) - 1.1547005384f * n_rotateMatrix.data()[2] * n_rotateMatrix.data()[8]) * inLightCoefficient[7];
            outLightCoefficient[6] += (0.2886751347f * (coeff[9] - coeff[8] + coeff[10] - coeff[11]) - 0.5773502692f *
                                                                                                       (n_rotateMatrix.data()[5] * n_rotateMatrix.data()[5] - n_rotateMatrix.data()[2] * n_rotateMatrix.data()[2])) * inLightCoefficient[8];

            outLightCoefficient[7] = (n_rotateMatrix.data()[0] * n_rotateMatrix.data()[8] + n_rotateMatrix.data()[2] * n_rotateMatrix.data()[6]) * inLightCoefficient[7];
            outLightCoefficient[7] -= (n_rotateMatrix.data()[3] * n_rotateMatrix.data()[2] + n_rotateMatrix.data()[5] * n_rotateMatrix.data()[0]) * inLightCoefficient[4];
            outLightCoefficient[7] += (n_rotateMatrix.data()[3] * n_rotateMatrix.data()[8] + n_rotateMatrix.data()[5] * n_rotateMatrix.data()[6]) * inLightCoefficient[5];
            outLightCoefficient[7] -= 1.7320508076f * n_rotateMatrix.data()[8] * n_rotateMatrix.data()[6] * inLightCoefficient[6];
            outLightCoefficient[7] -= (n_rotateMatrix.data()[0] * n_rotateMatrix.data()[2] - n_rotateMatrix.data()[3] * n_rotateMatrix.data()[5]) * inLightCoefficient[8];

            outLightCoefficient[8] = 0.5f * (coeff[11] - coeff[8] - coeff[9] + coeff[10]) * inLightCoefficient[8];
            outLightCoefficient[8] += (coeff[0] - coeff[1]) * inLightCoefficient[4];
            outLightCoefficient[8] += (coeff[2] - coeff[3]) * inLightCoefficient[5];
            outLightCoefficient[8] += 0.86602540f * (coeff[4] - coeff[5]) * inLightCoefficient[6];
            outLightCoefficient[8] += (coeff[7] - coeff[6]) * inLightCoefficient[7];
        }
        return outLightCoefficient;
    }

    if (fabsf(n_rotateMatrix.data()[8]) != 1.0f) {
        sinb = sqrtf(1.0f - n_rotateMatrix.data()[8] * n_rotateMatrix.data()[8]);
        alpha = atan2f(n_rotateMatrix.data()[7] / sinb, n_rotateMatrix.data()[6] / sinb);
        beta = atan2f(sinb, n_rotateMatrix.data()[8]);
        gamma = atan2f(n_rotateMatrix.data()[5] / sinb, -n_rotateMatrix.data()[2] / sinb);
    }
    else {
        alpha = atan2f(n_rotateMatrix.data()[1], n_rotateMatrix.data()[0]);
        beta = 0.0f;
        gamma = 0.0f;
    }

    SHRotateZ(temp, band, gamma, inLightCoefficient);
    rotate_X(temp1, band, 1.0f, temp);
    SHRotateZ(temp, band, beta, temp1);
    rotate_X(temp1, band, -1.0f, temp);
    SHRotateZ(outLightCoefficient, band, alpha, temp1);

    return outLightCoefficient;
}

void SHRotation::rotate_X(QVector<QVector3D> &out, unsigned int band, float a, QVector<QVector3D> &inLightCoefficient) {
    out[0] = inLightCoefficient[0];

    out[1] = a * inLightCoefficient[2];
    out[2] = -a * inLightCoefficient[1];
    out[3] = inLightCoefficient[3];

    out[4] = a * inLightCoefficient[7];
    out[5] = -inLightCoefficient[5];
    out[6] = -0.5f * inLightCoefficient[6] - 0.8660253882f * inLightCoefficient[8];
    out[7] = -a * inLightCoefficient[4];
    out[8] = -0.8660253882f * inLightCoefficient[6] + 0.5f * inLightCoefficient[8];
    out[9] = -a * 0.7905694842f * inLightCoefficient[12] + a * 0.6123724580f * inLightCoefficient[14];

    out[10] = -inLightCoefficient[10];
    out[11] = -a * 0.6123724580f * inLightCoefficient[12] - a * 0.7905694842f * inLightCoefficient[14];
    out[12] = a * 0.7905694842f * inLightCoefficient[9] + a * 0.6123724580f * inLightCoefficient[11];
    out[13] = -0.25f * inLightCoefficient[13] - 0.9682458639f * inLightCoefficient[15];
    out[14] = -a * 0.6123724580f * inLightCoefficient[9] + a * 0.7905694842f * inLightCoefficient[11];
    out[15] = -0.9682458639f * inLightCoefficient[13] + 0.25f * inLightCoefficient[15];
    if (band == 4)
        return;

    out[16] = -a * 0.9354143739f * inLightCoefficient[21] + a * 0.3535533845f * inLightCoefficient[23];
    out[17] = -0.75f * inLightCoefficient[17] + 0.6614378095f * inLightCoefficient[19];
    out[18] = -a * 0.3535533845f * inLightCoefficient[21] - a * 0.9354143739f * inLightCoefficient[23];
    out[19] = 0.6614378095f * inLightCoefficient[17] + 0.75f * inLightCoefficient[19];
    out[20] = 0.375f * inLightCoefficient[20] + 0.5590170026f * inLightCoefficient[22] + 0.7395099998f * inLightCoefficient[24];
    out[21] = a * 0.9354143739f * inLightCoefficient[16] + a * 0.3535533845f * inLightCoefficient[18];
    out[22] = 0.5590170026f * inLightCoefficient[20] + 0.5f * inLightCoefficient[22] - 0.6614378691f * inLightCoefficient[24];
    out[23] = -a * 0.3535533845f * inLightCoefficient[16] + a * 0.9354143739f * inLightCoefficient[18];
    out[24] = 0.7395099998f * inLightCoefficient[20] - 0.6614378691f * inLightCoefficient[22] + 0.125f * inLightCoefficient[24];
    if (band == 5)
        return;

    out[25] = a * 0.7015607357f * inLightCoefficient[30] - a * 0.6846531630f * inLightCoefficient[32] + a * 0.1976423711f * inLightCoefficient[34];
    out[26] = -0.5f * inLightCoefficient[26] + 0.8660253882f * inLightCoefficient[28];
    out[27] = a * 0.5229125023f * inLightCoefficient[30] + a * 0.3061861992f * inLightCoefficient[32] - a * 0.7954951525f * inLightCoefficient[34];
    out[28] = 0.8660253882f * inLightCoefficient[26] + 0.5f * inLightCoefficient[28];
    out[29] = a * 0.4841229022f * inLightCoefficient[30] + a * 0.6614378691f * inLightCoefficient[32] + a * 0.5728219748f * inLightCoefficient[34];
    out[30] = -a * 0.7015607357f * inLightCoefficient[25] - a * 0.5229125023f * inLightCoefficient[27] - a * 0.4841229022f * inLightCoefficient[29];
    out[31] = 0.125f * inLightCoefficient[31] + 0.4050463140f * inLightCoefficient[33] + 0.9057110548f * inLightCoefficient[35];
    out[32] = a * 0.6846531630f * inLightCoefficient[25] - a * 0.3061861992f * inLightCoefficient[27] - a * 0.6614378691f * inLightCoefficient[29];
    out[33] = 0.4050463140f * inLightCoefficient[31] + 0.8125f * inLightCoefficient[33] - 0.4192627370f * inLightCoefficient[35];
    out[34] = -a * 0.1976423711f * inLightCoefficient[25] + a * 0.7954951525f * inLightCoefficient[27] - a * 0.5728219748f * inLightCoefficient[29];
    out[35] = 0.9057110548f * inLightCoefficient[31] - 0.4192627370f * inLightCoefficient[33] + 0.0624999329f * inLightCoefficient[35];

}

void SHRotation::SHRotateZ(QVector<QVector3D> &out, unsigned int band, float angle, QVector<QVector3D> &inLightCoefficient) {
    int i, sum = 0;
    float c[5], s[5];

    out[0] = inLightCoefficient[0];

    for (i = 1; i < band; i++) {
        int j;

        c[i - 1] = cosf(i * angle);
        s[i - 1] = sinf(i * angle);
        sum += i * 2;

        out[sum - i] = c[i - 1] * inLightCoefficient[sum - i];
        out[sum - i] += s[i - 1] * inLightCoefficient[sum + i];
        for (j = i - 1; j > 0; j--)
        {
            out[sum - j] = QVector3D(0.0, 0.0, 0.0);
            out[sum - j] = c[j - 1] * inLightCoefficient[sum - j];
            out[sum - j] += s[j - 1] * inLightCoefficient[sum + j];
        }

        if (inLightCoefficient == out)
            out[sum] = QVector3D(0.0, 0.0, 0.0);
        else
            out[sum] = inLightCoefficient[sum];

        for (j = 1; j < i; j++)
        {
            out[sum + j] = QVector3D(0.0, 0.0, 0.0);;
            out[sum + j] = -s[j - 1] * inLightCoefficient[sum - j];
            out[sum + j] += c[j - 1] * inLightCoefficient[sum + j];
        }
        out[sum + i] = -s[i - 1] * inLightCoefficient[sum - i];
        out[sum + i] += c[i - 1] * inLightCoefficient[sum + i];
    }
}
