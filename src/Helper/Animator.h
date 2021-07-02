#ifndef QTREFERENCE_ANIMATOR_H
#define QTREFERENCE_ANIMATOR_H

#include "Animation.h"

#include <QMatrix4x4>
#include <QVector>

class Animation;

class Animator {
public:
    Animator() = default;
    explicit Animator(Animation* current, int& boneCount);
    void updateAnimation(float dt);
    void calculateBoneTransform(const AssimpNodeData* node, QMatrix4x4 parentTransform);
    QVector<QMatrix4x4> getPoseTransforms() {
        return m_Transforms;
    }

private:
    QVector<QMatrix4x4> m_Transforms;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentFrame;
    float m_DeltaTime;
};


#endif
