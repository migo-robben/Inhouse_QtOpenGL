#ifndef INHOUSE_QTOPENGL_AXISSYSTEM_H
#define INHOUSE_QTOPENGL_AXISSYSTEM_H

#include <iostream>

#include <QtMath>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QScopedPointer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

struct AxisData
{
    QVector3D position;
};

class Camera;
class AxisSystem :protected QOpenGLFunctions_4_5_Core {
public:
    AxisSystem();
    ~AxisSystem() override;

    QVector<AxisData> GetAxisData();
    void drawGeometry(
            QOpenGLShaderProgram *program,
            QMatrix4x4 model,
            QMatrix4x4 view,
            QMatrix4x4 projection);

    void initGeometry();
    void setupAttributePointer(QOpenGLShaderProgram *program);

private:
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QMatrix4x4 mvpMatrix;
};


#endif //INHOUSE_QTOPENGL_AXISSYSTEM_H
