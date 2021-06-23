#ifndef INHOUSE_QTOPENGL_AXISSYSTEM_H
#define INHOUSE_QTOPENGL_AXISSYSTEM_H

#include <iostream>

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
    ~AxisSystem();
    QVector<AxisData> GetAxisData();
    void drawGeometry(QOpenGLShaderProgram *program,
                                    QMatrix4x4 model,
                                    QMatrix4x4 view,
                                    QMatrix4x4 projection,
                                    Camera* camera);

    void initGeometry();
    void setupAttributePointer(QOpenGLShaderProgram *program);

private:
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QVector <AxisData> axisData;
    QMatrix4x4 mvpMatrix;
};


#endif //INHOUSE_QTOPENGL_AXISSYSTEM_H
