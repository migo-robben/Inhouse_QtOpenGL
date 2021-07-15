#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>


struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
    QVector3D normal;
    QVector3D tangent;
    QVector3D bitangent;
    QVector3D m_BlendShape1;
    QVector3D m_BlendShape2;
    QVector3D m_BlendShape3;
    QVector3D m_BlendShape4;
    QVector3D m_BlendShapeNormal1;
    QVector3D m_BlendShapeNormal2;
    QVector3D m_BlendShapeNormal3;
    QVector3D m_BlendShapeNormal4;
    QVector4D m_BoneIDs;
    QVector4D m_Weights;
    QVector3D ObjectSHCoefficient[9];
};


struct BlendShapePosition{
    unsigned int m_numAnimPos = 0;
    QVector<QVector3D> m_AnimPos;
};


class Geometry : protected QOpenGLFunctions_4_5_Core {
public:
    Geometry();
    virtual ~Geometry();

    virtual void initGeometry() = 0;
    virtual void setupAttributePointer(QOpenGLShaderProgram *program) = 0;

    virtual void drawGeometry(QOpenGLShaderProgram *program,
            QMatrix4x4 model,
            QMatrix4x4 view,
            QMatrix4x4 projection,
            QOpenGLTexture *texture) = 0;

    virtual void drawGeometry(QOpenGLShaderProgram *program,
            QOpenGLTexture *texture) = 0;

    int VerticesCount() { return getIndices().count(); }

protected:
    virtual QVector<VertexData> getVerticesData() = 0;
    virtual QVector<GLuint> getIndices() = 0;

protected:
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
};


#endif
