#ifndef INHOUSE_QTOPENGL_CUSTOMGEOMETRY_H
#define INHOUSE_QTOPENGL_CUSTOMGEOMETRY_H


#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "Helper/Geometry.h"

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

class Geometry;

class CustomGeometry : public Geometry {
public:
    CustomGeometry() = default;
    explicit CustomGeometry(QString  path);
    ~CustomGeometry() override = default;

    void initGeometry() override;
    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

    void processNode(aiNode *node, const aiScene *scene);
    void processMesh(aiMesh *mesh, const aiScene *scene);

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;

public:
    QString modelFilePath;
};


#endif
