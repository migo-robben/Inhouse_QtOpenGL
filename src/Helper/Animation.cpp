#include "Animation.h"
#include "CustomGeometry.h"

Animation::Animation(QString &animationPath, CustomGeometry* model) {
    // read file via ASSIMP
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
    const aiScene* scene = importer.ReadFile(animationPath.toStdString(), aiProcess_Triangulate);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        qDebug() << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }

    readHierarchyData(m_RootNode, scene->mRootNode);

    if(scene->mNumAnimations){
        auto animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;

        m_TicksPerSecond = animation->mTicksPerSecond;
        setupBones(animation, model);
        setupBlendShape(animation, model);

        qDebug() << "Duration:" << m_Duration << "," << "Fps:" << m_TicksPerSecond;
    }
}

void Animation::readHierarchyData(AssimpNodeData &dest, const aiNode *src) {
    dest.name = QString(src->mName.data);
    dest.transformation = convertAIMatrixToQtFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++) {
        AssimpNodeData newData;
        readHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

QMatrix4x4 Animation::convertAIMatrixToQtFormat(const aiMatrix4x4 &from) {
    QMatrix4x4 to;

    to.setRow(0, QVector4D(from.a1, from.a2, from.a3, from.a4));
    to.setRow(1, QVector4D(from.b1, from.b2, from.b3, from.b4));
    to.setRow(2, QVector4D(from.c1, from.c2, from.c3, from.c4));
    to.setRow(3, QVector4D(from.d1, from.d2, from.d3, from.d4));

    return to;
}

void Animation::setupBones(const aiAnimation *animation, CustomGeometry* model) {
    int size = animation->mNumChannels;
    qDebug() << "animation num channels: " << size;
    auto& boneInfoMap = model->getOffsetMatMap();
    int& boneCount = model->getBoneCount();

    for (int i = 0; i < size; i++) {
        auto channel = animation->mChannels[i];
        QString boneName(channel->mNodeName.data);

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }

        m_Bones.push_back(Bone(boneName, boneInfoMap[channel->mNodeName.data].id, channel));
    }

    m_BoneInfoMap = boneInfoMap;
}

void Animation::setupBlendShape(const aiAnimation *animation, CustomGeometry *model) {
    qDebug() << "NumMorphMeshChannels: " << animation->mNumMorphMeshChannels;
    m_keyMorph.resize(animation->mNumMorphMeshChannels);

    if(animation->mNumMorphMeshChannels){

        for(int j=0;j<animation->mNumMorphMeshChannels;j++){
        auto& channel = animation->mMorphMeshChannels[j];
            if(channel->mNumKeys) {
                QVector<KeyMorph> keyMorph;
                for (int i = 0; i < channel->mNumKeys; ++i) {
                    KeyMorph morph{};
                    auto &key = channel->mKeys[i];
                    morph.m_NumValuesAndWeights = key.mNumValuesAndWeights;  // same with bs length
                    morph.m_Time = key.mTime;
                    morph.m_Weights = new double[key.mNumValuesAndWeights];
                    morph.m_Values = new unsigned int[key.mNumValuesAndWeights];
                    std::copy(key.mWeights, key.mWeights + key.mNumValuesAndWeights, morph.m_Weights);
                    std::copy(key.mValues, key.mValues + key.mNumValuesAndWeights, morph.m_Values);
                    keyMorph.push_back(morph);
                }
                m_keyMorph[j] = keyMorph;
            }
        }
    }
}

Bone *Animation::FindBone(const QString &name) {
    auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& Bone)
        {
            return Bone.getBoneName() == name;
        }
    );
    if (iter == m_Bones.end())
        return nullptr;
    else
        return &(*iter);
}
