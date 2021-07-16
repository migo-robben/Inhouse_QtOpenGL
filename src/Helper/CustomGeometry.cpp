#include "CustomGeometry.h"
#include "cmath"


CustomGeometry::CustomGeometry(QString  path) : modelFilePath(std::move(path)) {
}

void CustomGeometry::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Offset for position
    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    int coordLocation = program->attributeLocation("aCoord");
    program->enableAttributeArray(coordLocation);
    program->setAttributeBuffer(coordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Offset for normal
    offset += sizeof(QVector2D);

    int normalLocation = program->attributeLocation("aNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for tangent
    offset += sizeof(QVector3D);

    int tangentLocation = program->attributeLocation("aTangent");
    program->enableAttributeArray(tangentLocation);
    program->setAttributeBuffer(tangentLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for bitangent
    offset += sizeof(QVector3D);

    int bitangentLocation = program->attributeLocation("aBitangent");
    program->enableAttributeArray(bitangentLocation);
    program->setAttributeBuffer(bitangentLocation, GL_FLOAT, offset, 3, sizeof(VertexData));


    // Offset for BlendShape
    unsigned int leftNumBlendShape = m_MaxNumBlendShape - m_NumBlendShape;
    for(int i=1;i <= m_NumBlendShape; i++){
        offset += sizeof(QVector3D);
        QString bsNum = QString("BlendShapeDeltaPos") + QString::number(i);
        int bsLocation = program->attributeLocation(bsNum);
        program->enableAttributeArray(bsLocation);
        program->setAttributeBuffer(bsLocation, GL_FLOAT, offset, 3, sizeof(VertexData));
    }
    offset += sizeof(QVector3D) * leftNumBlendShape;

    // Offset BlendShape Normal
    for(int i=1;i <= m_NumBlendShape; i++){
        offset += sizeof(QVector3D);
        QString bsNum = QString("BlendShapeDeltaNormal") + QString::number(i);
        int bsLocation = program->attributeLocation(bsNum);
        program->enableAttributeArray(bsLocation);
        program->setAttributeBuffer(bsLocation, GL_FLOAT, offset, 3, sizeof(VertexData));
    }
    offset += sizeof(QVector3D) * leftNumBlendShape;

    // Offset for boneIds
    offset += sizeof(QVector3D);

    int boneIdsLocation = program->attributeLocation("boneIds");
    program->enableAttributeArray(boneIdsLocation);
    program->setAttributeBuffer(boneIdsLocation, GL_FLOAT, offset, 4, sizeof(VertexData));

    // Offset for weights
    offset += sizeof(QVector4D);

    int weightsLocation = program->attributeLocation("weights");
    program->enableAttributeArray(weightsLocation);
    program->setAttributeBuffer(weightsLocation, GL_FLOAT, offset, 4, sizeof(VertexData));

    program->release();
}

void CustomGeometry::setupAttributePointer(QOpenGLShaderProgram *program, bool RPT, int bandPower2) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    int coordLocation = program->attributeLocation("aCoord");
    program->enableAttributeArray(coordLocation);
    program->setAttributeBuffer(coordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Offset for normal
    offset += sizeof(QVector2D);

    int normalLocation = program->attributeLocation("aNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for tangent
    offset += sizeof(QVector3D);

    int tangentLocation = program->attributeLocation("aTangent");
    program->enableAttributeArray(tangentLocation);
    program->setAttributeBuffer(tangentLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for bitangent
    offset += sizeof(QVector3D);

    int bitangentLocation = program->attributeLocation("aBitangent");
    program->enableAttributeArray(bitangentLocation);
    program->setAttributeBuffer(bitangentLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    //
    offset += sizeof(QVector3D);

    int ObjSHCoeffLocation = program->attributeLocation("ObjectSHCoefficient");
    for (int attrib = 0; attrib < bandPower2; attrib++) {
        program->setAttributeBuffer(
                ObjSHCoeffLocation + attrib,
                GL_FLOAT,
                offset + attrib * sizeof(QVector3D),
                3,
                sizeof(VertexData));
        program->enableAttributeArray(ObjSHCoeffLocation + attrib);
    }

    program->release();
}

void CustomGeometry::initGeometry() {
    // read file via ASSIMP
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
//    const aiScene* scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_JoinIdenticalVertices);
    const aiScene* scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        qDebug() << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }

    m_indexIncrease = 0;

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);

    qDebug() << "BlendShape Num: " << m_NumBlendShape;
    qDebug() << "Vertices Count: " << getVerticesData().length();

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void CustomGeometry::initAnimation() {
    animation = Animation(modelFilePath, this);
}

void CustomGeometry::initAnimator() {
    animator = Animator(&animation, this, getBoneCount());
}

void CustomGeometry::initGeometry(QVector<QVector<QVector3D>> &ObjectSHCoefficient) {
    // read file via ASSIMP
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
    const aiScene* scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_JoinIdenticalVertices);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        qDebug() << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
    setupObjectSHCoefficient(ObjectSHCoefficient);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void CustomGeometry::drawGeometry(QOpenGLShaderProgram *program,
                                QMatrix4x4 model,
                                QMatrix4x4 view,
                                QMatrix4x4 projection,
                                QOpenGLTexture *texture) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);
    program->setUniformValue("AlbedoMap", 0);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    texture->bind();
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

void
CustomGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

void CustomGeometry::drawGeometry(QOpenGLShaderProgram *program, QOpenGLTexture *texture) {
}

QVector<VertexData> CustomGeometry::getVerticesData() {
    return vertices;
}

QVector<GLuint> CustomGeometry::getIndices() {
    return indices;
}

void CustomGeometry::setVertexBoneDataToDefault(VertexData &data) {
//    for (int i = 0; i < MAX_BONE_WEIGHTS; i++) {
//        data.m_BoneIDs.push_back(-1);
//        data.m_Weights.push_back(0.0);
//    }

    data.m_BoneIDs.setX(-1);
    data.m_BoneIDs.setY(-1);
    data.m_BoneIDs.setZ(-1);
    data.m_BoneIDs.setW(-1);

    data.m_Weights.setX(0.0);
    data.m_Weights.setY(0.0);
    data.m_Weights.setZ(0.0);
    data.m_Weights.setW(0.0);
}

void CustomGeometry::setVertexBoneData(VertexData& vertex, int boneID, float weight) {
//    for (int i = 0; i < MAX_BONE_WEIGHTS; ++i) {
//        if (vertex.m_BoneIDs[i] < 0) {
//            vertex.m_BoneIDs[i] = boneID;
//            vertex.m_Weights[i] = weight;
//            break;
//        }
//    }
//    qDebug() << "boneID " << boneID;
    if (vertex.m_BoneIDs.x() < 0) {
        vertex.m_BoneIDs.setX(boneID);
        vertex.m_Weights.setX(weight);
    } else if (vertex.m_BoneIDs.y() < 0) {
        vertex.m_BoneIDs.setY(boneID);
        vertex.m_Weights.setY(weight);
    } else if (vertex.m_BoneIDs.z() < 0) {
        vertex.m_BoneIDs.setZ(boneID);
        vertex.m_Weights.setZ(weight);
    } else {
        vertex.m_BoneIDs.setW(boneID);
        vertex.m_Weights.setW(weight);
    }
}

QMatrix4x4 CustomGeometry::convertAIMatrixToQtFormat(const aiMatrix4x4& from) {
    QMatrix4x4 to;

    to.setRow(0, QVector4D(from.a1, from.a2, from.a3, from.a4));
    to.setRow(1, QVector4D(from.b1, from.b2, from.b3, from.b4));
    to.setRow(2, QVector4D(from.c1, from.c2, from.c3, from.c4));
    to.setRow(3, QVector4D(from.d1, from.d2, from.d3, from.d4));

    return to;
}

void CustomGeometry::extractBoneWeightForVertices(QVector<VertexData> &data, aiMesh* mesh, const aiScene* scene) {
    auto& boneInfoMap = m_OffsetMatMap;
    int& boneCount = m_BoneCount;

    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        QString boneName(mesh->mBones[boneIndex]->mName.C_Str());
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            BoneInfo newBoneInfo;

            newBoneInfo.id = boneCount;
            newBoneInfo.offset = convertAIMatrixToQtFormat(mesh->mBones[boneIndex]->mOffsetMatrix);

            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCount;
            boneCount++;
        }
        else {
            boneID = boneInfoMap[boneName].id;
        }

        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;
        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            setVertexBoneData(data[vertexId], boneID, weight);
        }
    }
}

void CustomGeometry::processNode(aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void CustomGeometry::processMesh(aiMesh *mesh, const aiScene *scene) {
    float maxBs = -1.0;
    // Walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        QVector3D pos;
        QVector2D tex;
        QVector3D normal;
        QVector3D tangent;
        QVector3D bitangent;
        QVector3D bsPos;
        VertexData data;

        setVertexBoneDataToDefault(data);

        pos.setX(mesh->mVertices[i].x);
        pos.setY(mesh->mVertices[i].y);
        pos.setZ(mesh->mVertices[i].z);

        bsPos = pos;

        if (mesh->mTextureCoords[0]) {
            tex.setX(mesh->mTextureCoords[0][i].x);
            tex.setY(mesh->mTextureCoords[0][i].y);
        }
        else {
            tex.setX(0.0f);
            tex.setY(0.0f);
        }

        if (mesh->mNormals) {
            normal.setX(mesh->mNormals[i].x);
            normal.setY(mesh->mNormals[i].y);
            normal.setZ(mesh->mNormals[i].z);
        }

        tangent.setX(mesh->mTangents[i].x);
        tangent.setY(mesh->mTangents[i].y);
        tangent.setZ(mesh->mTangents[i].z);

        bitangent.setX(mesh->mBitangents[i].x);
        bitangent.setY(mesh->mBitangents[i].y);
        bitangent.setZ(mesh->mBitangents[i].z);

        data.position = pos;
        data.texCoord = tex;
        data.normal = normal;
        data.tangent = tangent;
        data.bitangent = bitangent;

        qDebug() << i << pos;

        // blendShape
        BlendShapePosition bsp;
        bsp.m_numAnimPos = mesh->mNumAnimMeshes;
        m_NumBlendShape = bsp.m_numAnimPos;
        if(mesh->mNumAnimMeshes){
            for(unsigned int b=0; b<mesh->mNumAnimMeshes;++b){
                QVector3D b_pos, b_nor;
                aiVector3D b_ver = mesh->mAnimMeshes[b]->mVertices[i];
                aiVector3D b_nml = mesh->mAnimMeshes[b]->mNormals[i];
                b_pos.setX(b_ver.x);
                b_pos.setY(b_ver.y);
                b_pos.setZ(b_ver.z);
                b_nor.setX(b_nml.x);
                b_nor.setY(b_nml.y);
                b_nor.setZ(b_nml.z);
                qDebug() << "-- " << b_pos;
                maxBs = std::max(maxBs, b_pos.y());
                QVector3D deltaPos = b_pos - pos;
                QVector3D deltaNor = b_nor - normal;
                float maximumScaleFactor = 0.0f, minimumScaleFactor = 0.0f;
                maximumScaleFactor = std::max(maximumScaleFactor, deltaPos.x());
                maximumScaleFactor = std::max(maximumScaleFactor, deltaPos.y());
                maximumScaleFactor = std::max(maximumScaleFactor, deltaPos.z());
                minimumScaleFactor = std::min(minimumScaleFactor, deltaPos.x());
                minimumScaleFactor = std::min(minimumScaleFactor, deltaPos.y());
                minimumScaleFactor = std::min(minimumScaleFactor, deltaPos.z());
                scaleFactor = std::max(scaleFactor, std::max(maximumScaleFactor, std::abs(minimumScaleFactor)));
                bsp.m_AnimDeltaPos.push_back(deltaPos);
                bsp.m_AnimDeltaNor.push_back(deltaNor);
                if(b==0){
                    data.m_BlendShapeDeltaPos1 = b_pos - pos;
                    data.m_BlendShapeDeltaNormal1 = b_nor-normal;
                }
                if(b==1){
                    data.m_BlendShapeDeltaPos2 = b_pos - pos;
                    data.m_BlendShapeDeltaNormal2 = b_nor-normal;
                }
                if(b==2){
                    data.m_BlendShapeDeltaPos3 = b_pos-pos;
                    data.m_BlendShapeDeltaNormal3 = b_nor-normal;
                }
                if(b==3){
                    data.m_BlendShapeDeltaPos4 = b_pos-pos;
                    data.m_BlendShapeDeltaNormal4 = b_nor-normal;
                }
            }
        }
        assert(bsp.m_numAnimPos == bsp.m_AnimDeltaPos.length());
        m_blendShapeData.push_back(bsp);

        vertices.push_back(data);
    }
    qDebug() << maxBs;
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j] + m_indexIncrease);
    }

    m_indexIncrease += mesh->mNumVertices;

    extractBoneWeightForVertices(vertices, mesh, scene);
}

void CustomGeometry::setupObjectSHCoefficient(QVector<QVector<QVector3D>> &ObjectSHCoefficient) {
    int numbersOfVertices = ObjectSHCoefficient.count();
    int bandPower2 = ObjectSHCoefficient.count() > 0 ? ObjectSHCoefficient[0].count() : 0;

#pragma omp parallel for
    for (int i = 0; i < numbersOfVertices; i++) {
        for (int j = 0; j < bandPower2; j++) {
            vertices[i].ObjectSHCoefficient[j] = ObjectSHCoefficient[i][j];
        }
    }
}
