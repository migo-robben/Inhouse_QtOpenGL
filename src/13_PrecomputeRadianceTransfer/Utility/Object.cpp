#include "Object.h"

void Object::initGeometry(const QString& modelFilePath, QVector3D albedo, bool calcTangent) {
    _albedo = albedo;
    int beginIndex = modelFilePath.toStdString().rfind('\\');
    int endIndex = modelFilePath.toStdString().rfind('.');
    fileName = QString(modelFilePath.toStdString().substr(beginIndex+1, endIndex-beginIndex-1).c_str());

    // read file via ASSIMP
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);

    const aiScene* scene;
    if (calcTangent)
        scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_JoinIdenticalVertices);
    else
        scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_JoinIdenticalVertices);

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene, calcTangent);
}

QVector<SampleVertexData> Object::getVerticesData() {
    return vertices;
}

QVector<quint64> Object::getIndices() {
    return indices;
}

void Object::processNode(aiNode *node, const aiScene *scene, bool calcTangent) {
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, calcTangent);
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, calcTangent);
    }
}

void Object::processMesh(aiMesh *mesh, const aiScene *scene, bool calcTangent) {
    // Walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        QVector3D pos;
        QVector2D tex;
        QVector3D normal;
        QVector3D tangent;
        QVector3D bitangent;
        SampleVertexData data;

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
        if (calcTangent) {
            tangent.setX(mesh->mTangents[i].x);
            tangent.setY(mesh->mTangents[i].y);
            tangent.setZ(mesh->mTangents[i].z);

            bitangent.setX(mesh->mBitangents[i].x);
            bitangent.setY(mesh->mBitangents[i].y);
            bitangent.setZ(mesh->mBitangents[i].z);

            data.tangent = tangent;
            data.bitangent = bitangent;
        }

        data.position = pos;
        data.texCoord = tex;
        data.normal = normal;

        vertices.push_back(data);
    }

    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
}