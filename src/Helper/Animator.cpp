#include "Animator.h"

Animator::Animator(Animation* current, int& boneCount) {
    m_CurrentAnimation = current;
    m_CurrentFrame = 0.0;
    m_Transforms.resize(boneCount);
}

void Animator::updateAnimation(float dt) {
    m_DeltaTime = dt;

    if (m_CurrentAnimation) {
        m_CurrentFrame += m_CurrentAnimation->getTicksPerSecond() * dt;
        m_CurrentFrame = fmod(m_CurrentFrame, m_CurrentAnimation->getDuration());
        calculateBoneTransform(&m_CurrentAnimation->getRootNode(), QMatrix4x4());
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
