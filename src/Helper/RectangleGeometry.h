#ifndef _RECTANGLEGEOMETRY_H_
#define _RECTANGLEGEOMETRY_H_

#include "Helper/Geometry.h"

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QScopedPointer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

class RectangleGeometry : public Geometry {
public:
    RectangleGeometry() = default;
    ~RectangleGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;

    void drawGeometry(QOpenGLShaderProgram *program,
                      QOpenGLTexture *texture);

    void drawGeometry(QOpenGLShaderProgram *program,
                      QOpenGLFramebufferObject *fbo,
                      bool saveToDisk);

    void drawGeometry(QOpenGLShaderProgram *program);

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;
};


#endif
