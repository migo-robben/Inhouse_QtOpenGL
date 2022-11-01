#ifndef INHOUSE_QTOPENGL_CUSTOMGEOMETRY_H
#define INHOUSE_QTOPENGL_CUSTOMGEOMETRY_H

#include <QtOpenGL/QOpenGLBuffer>
#include <QtOpenGL/QOpenGLVertexArrayObject>
#include <QtOpenGL/QOpenGLShaderProgram>

#include <QFile>
#include <QVector3D>
#include <QTextStream>

#ifndef __EMSCRIPTEN__
    #include <QtOpenGL/QOpenGLFunctions_4_5_Core>
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
    QVector3D normal;
    QVector3D tangent;
    QVector3D bitangent;
};

class CustomGeometry
#ifndef __EMSCRIPTEN__
    : protected QOpenGLFunctions_4_5_Core
#endif
{
public:
    CustomGeometry(QString path);
    ~CustomGeometry();

    void initGeometry();
    void setupAttributePointer(QOpenGLShaderProgram *program);

    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection);

protected:
    void processNode(aiNode *node, const aiScene *scene);
    void processMesh(aiMesh *mesh, const aiScene *scene);

    QVector<VertexData> getVerticesData();
    QVector<GLuint> getIndices();

protected:
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;

private:
    QVector<VertexData> vertices;
    QVector<GLuint> indices;

public:
    QString modelFilePath;
};


#endif
