#ifndef _SPHEREGEOMETRY_H_
#define _SPHEREGEOMETRY_H_

#include "Helper/Geometry.h"

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QScopedPointer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

class SphereGeometry : public Geometry {
public:
    SphereGeometry() = default;
    ~SphereGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;

    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection);

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

    void setupSphere();

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;
};


#endif
