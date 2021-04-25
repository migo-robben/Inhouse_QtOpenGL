#include "CustomGeometry.h"


CustomGeometry::CustomGeometry(QString path) : modelFilePath(std::move(path)) {
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
    const aiScene* scene = importer.ReadFile(modelFilePath.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        qDebug() << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
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

void CustomGeometry::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    glDrawElements(GL_TRIANGLES, VerticesCount(), GL_UNSIGNED_INT, (void*)0);
}

QVector<VertexData> CustomGeometry::getVerticesData() {
    return vertices;
}

QVector<GLuint> CustomGeometry::getIndices() {
    return indices;
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
