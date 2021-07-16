#include "Animator.h"
#include "CustomGeometry.h"

Animator::Animator(Animation* current, CustomGeometry* geometry, int& boneCount) {
    m_CurrentAnimation = current;
    m_CurrentFrame = 0.0;
    m_Transforms.resize(boneCount);
    m_Geometry = geometry;
}

void Animator::updateAnimation(float dt) {
    m_DeltaTime = dt;

    if (m_CurrentAnimation) {
        m_CurrentFrame += m_CurrentAnimation->getTicksPerSecond() * dt;
        m_CurrentFrame = fmod(m_CurrentFrame, m_CurrentAnimation->getDuration());
        calculateBoneTransform(&m_CurrentAnimation->getRootNode(), QMatrix4x4());
        calculateBlendShapePosition(*m_CurrentAnimation->getKeyMorph());
    }
}

void Animator::calculateBoneTransform(const AssimpNodeData *node, QMatrix4x4 parentTransform) {
    QString nodeName = node->name;
    QMatrix4x4 nodeTransform = node->transformation;

    Bone* bone = m_CurrentAnimation->FindBone(nodeName);

    if (bone) {
        bone->update(m_CurrentFrame);
        nodeTransform = bone->getLocalTransform();
    }

    QMatrix4x4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_CurrentAnimation->getBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
        int index = boneInfoMap[nodeName].id;
        QMatrix4x4 offset = boneInfoMap[nodeName].offset;
        m_Transforms[index] = globalTransformation * offset;
    }
    for (int i = 0; i < node->childrenCount; i++)
        calculateBoneTransform(&node->children[i], globalTransformation);
}

void Animator::calculateBlendShapePosition(QVector<KeyMorph> km ) {
    if(km.empty())
        return;

    int bsIndex0 = int(m_CurrentFrame);
    int bsIndex1 = int(m_CurrentFrame)+1;

    double lastTime = km[bsIndex0].m_Time;
    double nextTime = km[bsIndex1].m_Time;
    double scaleFactor = (m_CurrentFrame-lastTime)/(nextTime-lastTime);
    int bsWeightLength = km[bsIndex0].m_NumValuesAndWeights;
    for(int i=0;i<bsWeightLength;i++){
        if(i==0)
            bsWeight1 = km[bsIndex0].m_Weights[0];
        if(i==1)
            bsWeight2 = km[bsIndex0].m_Weights[1];
        if(i==2)
            bsWeight3 = km[bsIndex0].m_Weights[2];
        if(i==3)
            bsWeight4 = km[bsIndex0].m_Weights[3];
    }
//    qDebug() << bsWeight1 << bsWeight2 << bsWeight3 << bsWeight4;
}
