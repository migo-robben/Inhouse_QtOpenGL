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

    // Offset for blendShape data
    offset += sizeof(QVector3D);
    int bsdataLocation = program->attributeLocation("aBlendShapeData");
    program->enableAttributeArray(bsdataLocation);
    program->setAttributeBuffer(bsdataLocation, GL_FLOAT, offset, 4, sizeof(VertexData));

    // Offset for boneIds
    offset += sizeof(QVector4D);

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
    m_animationNum = scene->mNumAnimations;

    // compute geometry transformation for fbx
    computeGeometryHierarchy(scene->mRootNode, QMatrix4x4());

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);

    verticesCount = getVerticesData().length();
    indicesCount = getIndices().length();

    qDebug() << "Bones Name: " << animation.getBoneIDMap().keys();
    qDebug() << "NumAnimations: " << m_animationNum << " BoneCount: " << m_BoneCount;
    qDebug() << "Vertices Indices Count: " << verticesCount << indicesCount;
    qDebug() << "blendShapeSlice: " << blendShapeSlice;
    qDebug() << "verticesSlice: " << verticesSlice;
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

void CustomGeometry::initAllocate() {
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

        if (boneInfoMap.find(boneName) == boneInfoMap.end()) { // notfound
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
            int vertexId = weights[weightIndex].mVertexId+m_indexIncrease;
            float weight = weights[weightIndex].mWeight;
            // FIXME is that necessary for the line of the follow, or consider we added m_indexIncrease
            //assert(vertexId <= vertices.size());
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

void CustomGeometry::computeScaleFactor(QVector3D& v){
    float maxmFacX=0.0f, maxmFacY=0.0f,maxmFacZ=0.0f, minmFacX=0.0f, minmFacY=0.0f, minmFacZ=0.0f;
    maxmFacX = std::max(maxmFacX, v.x());
    maxmFacY = std::max(maxmFacY, v.y());
    maxmFacZ = std::max(maxmFacZ, v.z());
    minmFacX = std::min(minmFacX, v.x());
    minmFacY = std::min(minmFacY, v.y());
    minmFacZ = std::min(minmFacZ, v.z());
    scaleFactor.setX(std::max(scaleFactor.x(), std::max(maxmFacX, std::abs(minmFacX))));
    scaleFactor.setY(std::max(scaleFactor.y(), std::max(maxmFacY, std::abs(minmFacY))));
    scaleFactor.setZ(std::max(scaleFactor.z(), std::max(maxmFacZ, std::abs(minmFacZ))));
}

void CustomGeometry::processMesh(aiMesh *mesh, const aiScene *scene) {
    /*
     * Notes:
     *      FBX: if the mesh has no animation, no blendShape and no transformation and
     *          the vertices data will only record the local position, need to get the transformation
     *          from mRootNode -> mTransformation -> mChildren -> mTransformation ...
     *          except Freeze transformation before we exported;
     */
    float maxBs = -1.0;
    bool blendShapeUnsliced = true;
    m_blendShapeData.clear();
    int vlevel = computeLevelByVCount(mesh->mNumVertices, 2);
    mesh->mNumBones; // it will be 0 if this mesh that doesn't have any bone influence with.
                        // so we need multi-matrix transformation
    QString qmeshName = QString(mesh->mName.data);

    qDebug() << "circling mesh info: (name, numBone)" << qmeshName << mesh->mNumBones;
    // Walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        QVector3D pos;
        QVector2D tex;
        QVector3D normal;
        QVector3D tangent;
        QVector3D bitangent;
        QVector4D bsdata;
        VertexData data;

        setVertexBoneDataToDefault(data);

        pos.setX(mesh->mVertices[i].x);
        pos.setY(mesh->mVertices[i].y);
        pos.setZ(mesh->mVertices[i].z);

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

        bsdata.setX(mesh->mNumAnimMeshes);
        bsdata.setY(mesh->mNumVertices);
        bsdata.setZ(vlevel);
        bsdata.setW(0.0f);

        data.position = pos;
        data.texCoord = tex;
        data.normal = normal;
        data.tangent = tangent;
        data.bitangent = bitangent;
        data.bsdata = bsdata;

        // blendShape
        BlendShapePosition bsp;
        bsp.m_numAnimPos = mesh->mNumAnimMeshes;
        if(mesh->mNumAnimMeshes){
            unsigned int bsLen = mesh->mAnimMeshes[0]->mNumVertices;
            for(unsigned int b=0; b<mesh->mNumAnimMeshes;++b){
                QVector3D b_pos, b_nor;
                aiVector3D b_ver = mesh->mAnimMeshes[b]->mVertices[i];
                aiVector3D b_nml = mesh->mAnimMeshes[b]->mNormals[i];
                b_pos.setX(b_ver.x); b_pos.setY(b_ver.y); b_pos.setZ(b_ver.z);
                b_nor.setX(b_nml.x); b_nor.setY(b_nml.y); b_nor.setZ(b_nml.z);
                maxBs = std::max(maxBs, b_pos.y());
                QVector3D deltaPos = b_pos - pos;
                QVector3D deltaNor = b_nor - normal;
                computeScaleFactor(deltaPos);
                bsp.m_AnimDeltaPos.push_back(deltaPos);
                bsp.m_AnimDeltaNor.push_back(deltaNor);
            }
            assert(bsp.m_numAnimPos == bsp.m_AnimDeltaPos.length());

            m_blendShapeData.push_back(bsp);
            if(blendShapeUnsliced){
                blendShapeSlice.append(QVector4D(m_indexIncrease, m_indexIncrease+bsLen, m_BSID, 0));
                blendShapeUnsliced = false;
            }
        }

        vertices.push_back(data);
    }

    // push blendshape data
    if(mesh->mNumAnimMeshes){
        m_BSDATA.push_back(m_blendShapeData);
        ++m_BSID;
    }

    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j] + m_indexIncrease);
    }

    extractBoneWeightForVertices(vertices, mesh, scene);
    verticesSlice[qmeshName] = QVector<unsigned int>{static_cast<unsigned int>(m_indexIncrease),
                                                     m_indexIncrease + mesh->mNumVertices,
                                                     mesh->mNumBones};
    m_indexIncrease += mesh->mNumVertices;
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

int CustomGeometry::computeLevelByVCount(unsigned int vcount, int split_tile) {
    int precision = 0;
    for(int i=0; i<14;i++){  // 2^14=16384   16K max compute for now
        int apart = std::pow(2, i);
        if(apart*apart/split_tile > vcount){
            precision = apart;
            break;
        }
    }
    return precision;
}

void CustomGeometry::computeGeometryHierarchy(const aiNode * parentNode, QMatrix4x4 parentTransform) {
    unsigned int childrenCount = parentNode->mNumChildren;
    QMatrix4x4 transformation = convertAIMatrixToQtFormat(parentNode->mTransformation);

    QString name( parentNode->mName.data ) ;
    if (m_geoMatrix.find(name) == m_geoMatrix.end() && parentNode->mNumMeshes){
        m_geoMatrix[name] = QMatrix4x4();
    }

    transformation = parentTransform * transformation;

    if(parentNode->mNumMeshes){
        m_geoMatrix[name] = transformation * m_geoMatrix[name];
    }

    for (int i = 0; i < childrenCount; i++) {
        computeGeometryHierarchy(parentNode->mChildren[i], transformation);
    }
}

void CustomGeometry::setupTransformationAttribute() {
    QMap<QString, BoneInfo> boneInfo = animation.getBoneIDMap();  // for fbx transformation animation,
    // if the mesh name is same with bone name so we need to set BoneId and Weights as skeletal
    QMapIterator<QString, QVector<unsigned int>> iter(verticesSlice);
    while (iter.hasNext()) {
        iter.next();
        QString meshName = iter.key();
        QVector<unsigned int> verSlice = iter.value();
        unsigned int verStart = verSlice[0];
        unsigned int verEnd = verSlice[1];
        unsigned int verBoneNum = verSlice[2];
        bool foundMeshBone = boneInfo.find(meshName) != boneInfo.end();

        int meshBoneId = -1;
        float meshBoneWeight = 0.0f;
        if(foundMeshBone){
            meshBoneId = boneInfo[meshName].id;
            meshBoneWeight = 1.0f;
        }
        qDebug() << "setup trans attr: " << iter.key() << ": " << iter.value() << foundMeshBone << verBoneNum;
        for(unsigned int i=verStart;i<verEnd;i++){
            if(verBoneNum == 0){
                auto & vertice = vertices[i];

                if(foundMeshBone){
                    vertice.m_BoneIDs.setX(meshBoneId);
                    vertice.m_Weights.setX(meshBoneWeight);
                }else
                {  // if it wasn't effected by bone and transformation so we set parent transformation.
                    vertice.position = m_geoMatrix[meshName] * vertices[i].position;
                }
            }
        }
    }
}

