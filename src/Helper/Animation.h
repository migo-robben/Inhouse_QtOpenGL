#ifndef QTREFERENCE_ANIMATION_H
#define QTREFERENCE_ANIMATION_H

#include "Bone.h"
#include "assimp/scene.h"

#include <QString>
#include <QMatrix4x4>

class CustomGeometry;

struct AssimpNodeData
{
    QMatrix4x4 transformation;
    QString name;
    int childrenCount;
    QVector<AssimpNodeData> children;
};

class Animation {
public:
    Animation() = default;
    Animation(QString& animationPath, CustomGeometry* model);
    void readHierarchyData(AssimpNodeData& dest, const aiNode* src);
    void setupBones(const aiAnimation* animation, CustomGeometry* model);
    Bone* FindBone(const QString& name);

    QMatrix4x4 convertAIMatrixToQtFormat(const aiMatrix4x4& from);

    inline float getTicksPerSecond() { return m_TicksPerSecond; }
    inline float getDuration() { return m_Duration; }
    inline const AssimpNodeData& getRootNode() { return m_RootNode; }
    inline const QMap<QString, BoneInfo>& getBoneIDMap() { return m_BoneInfoMap; }

private:
    double m_Duration;
    double m_TicksPerSecond;

    AssimpNodeData m_RootNode;
    QVector<Bone> m_Bones;
    QMap<QString, BoneInfo> m_BoneInfoMap;
};

#endif
