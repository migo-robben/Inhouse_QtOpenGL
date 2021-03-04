#ifndef INHOUSE_QTOPENGL_GRIDGEOMETRY_H
#define INHOUSE_QTOPENGL_GRIDGEOMETRY_H

#include "Helper/Geometry.h"

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QScopedPointer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

class GridGeometry : public Geometry {
public:
    GridGeometry() = default;
    ~GridGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;

    void drawGeometry(QOpenGLShaderProgram *program);

    void drawGeometry(QOpenGLShaderProgram *program,
                      QOpenGLFramebufferObject *fbo,
                      bool saveToDisk);

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;
};


#endif
