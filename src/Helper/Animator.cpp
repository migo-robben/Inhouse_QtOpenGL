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
        calculateBlendShapePosition(m_CurrentAnimation->getKeyMorph());
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

    if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {  // found
        int index = boneInfoMap[nodeName].id;
        QMatrix4x4 offset = boneInfoMap[nodeName].offset;
        m_Transforms[index] = globalTransformation * offset;
    }
    for (int i = 0; i < node->childrenCount; i++)
        calculateBoneTransform(&node->children[i], globalTransformation);
}

void Animator::calculateBlendShapePosition(QVector<QVector<KeyMorph>> km ) {
    if(km.empty())
        return;
    if(bsWeights.count() > maxBlendShape){
        qDebug() << "Over maximum limit of blendShape";
        return;
    }
    bsWeights.clear();

    // TODO optimise interpolation
    int bsIndex0 = int(m_CurrentFrame);
    for(int i=0; i<km.length(); i++){
        QVector<KeyMorph> keysMorph = km[i];
        unsigned int bsWeightLength = keysMorph[bsIndex0].m_NumValuesAndWeights;  // same with bs length

        for(int j=0; j<bsWeightLength; j++){
            QVector2D weightData;
            weightData.setX( i );
            weightData.setY( keysMorph[bsIndex0].m_Weights[j] );
            bsWeights.append( weightData ); // combine them all into one array and will deal that in vs shader
        }
    }
}
