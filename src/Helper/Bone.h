#ifndef QTREFERENCE_BONE_H
#define QTREFERENCE_BONE_H

#include "assimp/scene.h"

#include <QString>
#include <QMatrix4x4>
#include <QVector3D>

struct BoneInfo {
    int id;
    QMatrix4x4 offset;
};

struct KeyPosition {
    QVector3D position;
    float timeStamp;
};

struct KeyRotation {
    QQuaternion orientation;
    float timeStamp;
};

struct KeyScale {
    QVector3D scale;
    float timeStamp;
};

class Bone {
public:
    Bone() = default;
    explicit Bone(QString& name, int ID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID) {

        // ----- Translate ----- //
        m_NumPositions = channel->mNumPositionKeys;
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;

            KeyPosition data;
            data.position = QVector3D(aiPosition.x, aiPosition.y, aiPosition.z);
            data.timeStamp = timeStamp;
            m_Positions.push_back(data);
        }

        // ----- Rotation ----- //
        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;

            KeyRotation data;
            data.orientation = QQuaternion(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
            data.timeStamp = timeStamp;
            m_Rotations.push_back(data);
        }

        // ----- Scaling ----- //
        m_NumScalings = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex) {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;

            KeyScale data;
            data.scale = QVector3D(scale.x, scale.y, scale.z);
            data.timeStamp = timeStamp;
            m_Scales.push_back(data);
        }
    }

    QMatrix4x4 getLocalTransform() { return m_LocalTransform; }
    QString getBoneName() const { return m_Name; }

    void update(float animationTime) {
        QMatrix4x4 translation = interpolatePosition(animationTime);
        QMatrix4x4 rotation = interpolateRotation(animationTime);
        QMatrix4x4 scale = interpolateScaling(animationTime);

        m_LocalTransform = translation * rotation * scale;
    }

    int getPositionIndex(float animationTime) {
        for (int index = 0; index < m_NumPositions - 1; ++index) {
            if (animationTime < m_Positions[index + 1].timeStamp)
                return index;
        }
    }
    int getRotationIndex(float animationTime) {
        for (int index = 0; index < m_NumRotations - 1; ++index) {
            if (animationTime < m_Rotations[index + 1].timeStamp)
                return index;
        }
    }
    int getScaleIndex(float animationTime) {
        for (int index = 0; index < m_NumScalings - 1; ++index) {
            if (animationTime < m_Scales[index + 1].timeStamp)
                return index;
        }
    }

private:
    QMatrix4x4 interpolatePosition(float animationTime) {
        QMatrix4x4 result;
        result.setToIdentity();

        if (1 == m_NumPositions) {
            result.translate(m_Positions[0].position);
            return result;
        }

        int p0Index = getPositionIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(
                m_Positions[p0Index].timeStamp,
                m_Positions[p1Index].timeStamp,
                animationTime);
        QVector3D finalPosition = m_Positions[p0Index].position * (1.0f - scaleFactor) + m_Positions[p1Index].position * scaleFactor;
        result.translate(finalPosition);

        return result;
    }

    QMatrix4x4 interpolateRotation(float animationTime) {
        QMatrix4x4 result;
        result.setToIdentity();

        if (1 == m_NumRotations) {
            result.rotate(m_Rotations[0].orientation.normalized());
            return result;
        }

        int p0Index = getRotationIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(
                m_Rotations[p0Index].timeStamp,
                m_Rotations[p1Index].timeStamp,
                animationTime);
        QQuaternion finalRotation = QQuaternion::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
        result.rotate(finalRotation.normalized());

        return result;
    }

    QMatrix4x4 interpolateScaling(float animationTime) {
        QMatrix4x4 result;
        result.setToIdentity();

        if (1 == m_NumScalings) {
            result.scale(m_Scales[0].scale);
            return result;
        }

        int p0Index = getScaleIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(
                m_Scales[p0Index].timeStamp,
                m_Scales[p1Index].timeStamp,
                animationTime);
        QVector3D finalScale = m_Scales[p0Index].scale *  (1.0f - scaleFactor) + m_Scales[p1Index].scale * scaleFactor;
        result.scale(finalScale);

        return result;
    }

    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;

        return scaleFactor;
    }

    QVector<KeyPosition> m_Positions;
    QVector<KeyRotation> m_Rotations;
    QVector<KeyScale> m_Scales;

    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    QMatrix4x4 m_LocalTransform;
    QString m_Name;
    int m_ID;
};

#endif
