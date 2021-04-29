#include <QSurfaceFormat>
#include <QApplication>
#include <QDebug>

#include "mainwidget.h"

enum Type {
    TYPE_UNSHADOW = 0,
    TYPE_SHADOW,
    TYPE_INTERREFLECTION
};

void processingData(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    if (argc >= 2) {
        processingData(argc, argv);
        qDebug() << "Processing Data Done";
    }

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("PRT UnShadow");

    QString LightFuncData;
    QString ObjectFuncData;
    switch(TYPE_SHADOW) {
        case 0:
            LightFuncData = QString("stpeters_probe.dat");
            ObjectFuncData = QString("buddhaDU.dat");
            break;
        case 1:
            LightFuncData = QString("stpeters_probe.dat");
            ObjectFuncData = QString("buddhaDS.dat");
            break;
        case 2:
            LightFuncData = QString("stpeters_probe.dat");
            ObjectFuncData = QString("buddhaDI.dat");
            break;
        default:
            break;
    }

    MainWidget w(LightFuncData, ObjectFuncData);
    w.show();

    return QApplication::exec();
}

void processingData(int argc, char *argv[]) {
    // -l for processing light function
    // -o for processing transfer function (object)
    std::string processingType(argv[1]);
    std::string generateType(argv[2]);

    // default number of sample, band
    int numbersOfSampler = 4096;
    int band = 3;

    if (processingType == "-l") {
        if (argc > 4) {
            band = atoi(argv[4]);
        }
        if (argc > 5) {
            numbersOfSampler = atoi(argv[5]);
        }

        if (generateType == "-p") {
            // light probe
            // .\PRT.exe -l -p beach_probe.hdr [band] [numbers of sampler]
            Lighting lightPattern(argv[3], PROBE, band);
            lightPattern.processingData(numbersOfSampler, true);
            lightPattern.saveToDisk(lightPattern.fileName + QString(".dat"));
        }
    }
    else if (processingType == "-o") {
        QVector3D albedo(0.15, 0.15, 0.15);
        // .\PRT.exe -o -d 1 buddha.obj [band] [sample number]

        int transferType = atoi(argv[3]);
        if (argc > 5) {
            band = atoi(argv[5]);
        }
        if (argc > 6) {
            numbersOfSampler = atoi(argv[6]);
        }

        if (generateType == "-d") {
            // Diffuse Object
            DiffuseObject diffuseObj;
            diffuseObj.initGeometry(QString(argv[4]), albedo, true);
            diffuseObj.processingData(transferType, band, numbersOfSampler, 1);
            diffuseObj.saveToDisk(diffuseObj.fileName + (transferType == 1 ? QString("DU.dat") : (transferType == 2 ? QString("DS.dat") : QString("DI.dat"))));
        }
        else if (generateType == "-g") {
            // Glossy Object
        }
    }
}