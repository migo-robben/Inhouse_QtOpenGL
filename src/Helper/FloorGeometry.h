#ifndef _FLOORGEOMETRY_H
#define _FLOORGEOMETRY_H

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "Helper/Geometry.h"

class Geometry;

class FloorGeometry : public Geometry {
public:
    FloorGeometry() = default;
    ~FloorGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QOpenGLTexture *texture);

    void drawGeometry(QOpenGLShaderProgram *program);

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;

};


#endif
