#ifndef INHOUSE_QTOPENGL_SKYBOXGEOMETRY_H
#define INHOUSE_QTOPENGL_SKYBOXGEOMETRY_H


#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "Helper/Geometry.h"

class Geometry;

class SkyboxGeometry : public Geometry {
public:
    SkyboxGeometry() = default;
    ~SkyboxGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;


    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture);


protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;
};


#endif
