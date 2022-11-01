#include "CustomGeometry.h"

CustomGeometry::CustomGeometry(QString path) : vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer), modelFilePath(path) {
#ifndef __EMSCRIPTEN__
    QOpenGLFunctions_4_5_Core::initializeOpenGLFunctions();
#endif
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    vbo.create();
    ebo.create();
}

CustomGeometry::~CustomGeometry()
{
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
}

void CustomGeometry::initGeometry() {
    QFile inputFile = QFile(modelFilePath);
    std::string data;

    if (inputFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&inputFile);
        data = in.readAll().toStdString();
    }

    // read file via ASSIMP
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
    // step to normalize all vertex components into the [-1,1] range.
    // That is, a bounding box for the whole scene is computed, the maximum component is taken and all meshes are scaled appropriately (uniformly of course!)
    const aiScene* scene = importer.ReadFileFromMemory(data.c_str(), data.size(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
#ifndef __EMSCRIPTEN__
        qDebug() << "ERROR::ASSIMP:: " << importer.GetErrorString();
#else
        EM_ASM( {console.log("ERROR:ASSIMP initGeometry.")}; );
#endif
        return;
    }

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
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

    program->release();
}

void CustomGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    glDrawElements(GL_TRIANGLES, getVerticesData().count(), GL_UNSIGNED_INT, (void*)0);
}

void CustomGeometry::processNode(aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void CustomGeometry::processMesh(aiMesh *mesh, const aiScene *scene) {
    // Walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        QVector3D pos;
        QVector2D tex;
        QVector3D normal;
        QVector3D tangent;
        QVector3D bitangent;
        VertexData data;

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

        data.position = pos;
        data.texCoord = tex;
        data.normal = normal;
        data.tangent = tangent;
        data.bitangent = bitangent;

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

QVector<VertexData> CustomGeometry::getVerticesData() {
    return vertices;
}

QVector<GLuint> CustomGeometry::getIndices() {
    return indices;
}