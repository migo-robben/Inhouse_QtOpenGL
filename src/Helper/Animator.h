#ifndef QTREFERENCE_ANIMATOR_H
#define QTREFERENCE_ANIMATOR_H

#include "Animation.h"

#include <QMatrix4x4>
#include <QVector>

class Animation;

class Animator {
public:
    Animator() = default;
    explicit Animator(Animation* current, CustomGeometry* geometry, int& boneCount);
    void updateAnimation(float dt);
    void calculateBoneTransform(const AssimpNodeData* node, QMatrix4x4 parentTransform);
    void calculateBlendShapePosition(QVector<KeyMorph>);
    QVector<QMatrix4x4> getPoseTransforms() {
        return m_Transforms;
    }

public:
    float bsWeight1 = 0.0f;
    float bsWeight2 = 0.0f;
    float bsWeight3 = 0.0f;
    float bsWeight4 = 0.0f;

private:
    QVector<QMatrix4x4> m_Transforms;
    CustomGeometry* m_Geometry;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentFrame;
    float m_DeltaTime;
};


#endif
