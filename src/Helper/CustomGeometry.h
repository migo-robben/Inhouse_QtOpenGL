#ifndef _CUSTOMGEOMETRY_H_
#define _CUSTOMGEOMETRY_H_

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "Helper/Geometry.h"
#include "Helper/Bone.h"
#include "Helper/Animation.h"
#include "Helper/Animator.h"

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#define MAX_BONE_WEIGHTS 4

class Geometry;
class Animation;
class Animator;

class CustomGeometry : public Geometry {
public:
    CustomGeometry() = default;
    explicit CustomGeometry(QString  path);
    ~CustomGeometry() override = default;

    void initGeometry() override;
    void initAnimation();
    void initAnimator();
    QMap<QString, BoneInfo>& getOffsetMatMap() { return m_OffsetMatMap; }
    int& getBoneCount() { return m_BoneCount; }
    void extractBoneWeightForVertices(QVector<VertexData> &data, aiMesh* mesh, const aiScene* scene);
    void setVertexBoneDataToDefault(VertexData &data);
    QMatrix4x4 convertAIMatrixToQtFormat(const aiMatrix4x4& from);
    void setVertexBoneData(VertexData& vertex, int boneID, float weight);
    void initGeometry(QVector<QVector<QVector3D>> &ObjectSHCoefficient);

    void setupAttributePointer(QOpenGLShaderProgram *program) override;
    void setupAttributePointer(QOpenGLShaderProgram *program, bool RPT, int bandPower2);
    static int computeLevelByVCount(unsigned int, int);
    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection,
                      QOpenGLTexture *texture) override;
    void drawGeometry(QOpenGLShaderProgram *program,
                      QOpenGLTexture *texture) override;

    void drawGeometry(QOpenGLShaderProgram *program,
                      QMatrix4x4 model,
                      QMatrix4x4 view,
                      QMatrix4x4 projection);

    void setupObjectSHCoefficient(QVector<QVector<QVector3D>> &ObjectSHCoefficient);

protected:
    QVector<VertexData> getVerticesData() override;
    QVector<GLuint> getIndices() override;

    void processNode(aiNode *node, const aiScene *scene);
    void processMesh(aiMesh *mesh, const aiScene *scene);

private:
    void computeScaleFactor(QVector3D&);
    QVector<VertexData> vertices;
    QVector<GLuint> indices;

    // ----- Animation ----- //
    QMap<QString, BoneInfo> m_OffsetMatMap;
    int m_BoneCount = 0;
    int m_indexIncrease = 0;
    QVector<BlendShapePosition> m_blendShapeData;
    QVector<QMatrix4x4> m_Transforms;
public:
    Animation animation;

public:
    unsigned int verticesCount;
    unsigned int indicesCount;

    unsigned int m_BSID = 0;
    QVector<QVector<BlendShapePosition>> m_BSDATA;

    QString modelFilePath;
    QVector3D scaleFactor;
    QVector<QVector4D> blendShapeSlice;
    Animator animator;
};


#endif
