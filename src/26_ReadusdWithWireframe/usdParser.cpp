#include "usdParser.h"

usdParser::usdParser(QString &path) : usdFilePath(path), ebo(QOpenGLBuffer::IndexBuffer) {
    stage = UsdStage::Open(usdFilePath.toStdString());

    // Base parameter
    fps = stage->GetFramesPerSecond();
    animStartFrame = stage->GetStartTimeCode();
    animEndFrame = stage->GetEndTimeCode();

    spdlog::info("\n\tParser usd file: {}\n\tAnimation start frame: {}\n\tAnimation end frame: {}",
                 usdFilePath.toStdString(),
                 animStartFrame, animEndFrame);

    QOpenGLFunctions_4_5_Core::initializeOpenGLFunctions();
    vaoGeometry.create();
    for (int i=0; i<attributeCount; ++i) {
        QOpenGLBuffer vbo;
        vbo.create();
        vbos.push_back(vbo);
    }
    ebo.create();

    // ----- wireframe ----- //
    vaoWireframe.create();
    vboWireframe.create();
}

void usdParser::getUVToken(UsdPrim &prim, TfToken &tf_uv, bool &uvs) {
    TfToken tf_st("primvars:st");
    TfToken tf_map1("primvars:map1");
    uvs = false;

    if (prim.HasProperty(tf_st)) {
        tf_uv = tf_st;
        uvs = true;
        return;
    } else if (prim.HasProperty(tf_map1)) {
        tf_uv = tf_map1;
        uvs = true;
        return;
    }
}

void usdParser::getDataBySpecifyFrame_TBB_with_simpleTriangulation(UsdTimeCode timeCode) {
    spdlog::info("\n\tGet data by specify frame: {}", timeCode.GetValue());

    currenTimeCode = timeCode;

    myTimer m_timer;
    m_timer.setStartPoint();
    if (!m_has_triangulated)
        m_indicesCount = 0;

    // ----- Mesh dictionary ----- //
    // { "mesh1":{0, 0, 4, 2}, "mesh2":{1, 4, 28, 14}, ... }
    // { "mesh_name": {mesh_index, indices_start, indices_end, the_number_of_triangles_accumulated_at_the_current_position}, ... }
    std::map<std::string, std::vector<int>> meshDict;

    // ----- Precalculate the number of triangulation and mesh index ----- //
    int totalTriangle = 0, meshIndex = 0, indicesSize = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() == "Mesh") {
            // Get Visibility
            UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
            TfToken visibility;
            attr_visibility.Get(&visibility, currenTimeCode);
            if (visibility == UsdGeomTokens->invisible) {
                continue;
            }

            std::vector<int> currentProcessMeshData;
            VtArray<int> vt_faceVertexCounts, vt_faceVertexIndices;

            UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
            UsdAttribute attr_faceVertexIndices = prim.GetAttribute(TfToken("faceVertexIndices"));

            attr_faceVertexCounts.Get(&vt_faceVertexCounts, currenTimeCode);
            attr_faceVertexIndices.Get(&vt_faceVertexIndices, currenTimeCode);

            currentProcessMeshData.emplace_back(meshIndex);
            currentProcessMeshData.emplace_back(indicesSize); // represent start pointer
            for (int vt_faceVertexCount : vt_faceVertexCounts) {
                totalTriangle += vt_faceVertexCount - 2;
            }
            indicesSize += int(vt_faceVertexIndices.size());
            currentProcessMeshData.emplace_back(indicesSize); // represent end pointer
            currentProcessMeshData.emplace_back(totalTriangle);

            meshDict[prim.GetPath().GetString()] = currentProcessMeshData;
            meshIndex++;
        }
    }

    // ----- Preallocate memory ----- //
    indices.resize(totalTriangle * 3); // 3 points per triangulation
    vt_gl_position.resize(indicesSize);
    vt_gl_texCoord.resize(indicesSize);
    vt_gl_normal.resize(indicesSize);

    // ----- Actually the number of points, uv, normal ----- //
    int actually_points = 0, actually_uv = 0, actually_normal = 0;

    // ----- Parallel traverse all prim ----- //
    tbb::parallel_do(
            stage->TraverseAll(),
            [this, &timeCode, &actually_points, &meshDict](UsdPrim prim) {
                if (prim.GetTypeName() != "Mesh") {
                    return;
                } else {
                    UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
                    TfToken visibility;
                    attr_visibility.Get(&visibility, currenTimeCode);
                    if (visibility == UsdGeomTokens->invisible) {
                        return;
                    }
                }

                spdlog::info("\n\t Processing mesh: {}", prim.GetPath().GetString());

                myTimer subtimer;
                subtimer.setStartPoint();

                VtArray<int> vt_faceVertexCounts;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
                attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

                std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
                // Range: [ start_pointer, end_pointer - 1 ]
                int start_pointer = mesh_data[1]; // include this value
                int end_pointer = mesh_data[2]; // except this value, end_pointer - 1;
                int number_of_triangulation = mesh_data[3];

                // Mesh Attribute Token
                TfToken tf_faceVertexIndices("faceVertexIndices");
                TfToken tf_normals("normals");
                TfToken tf_points("points");

                // Get UV Attribute Token
                bool has_uv;
                TfToken tf_uv;
                this->getUVToken(prim, tf_uv, has_uv);

                // Check Normal Attribute
                bool has_normal = prim.HasProperty(tf_normals);

                // USD Attributes
                VtArray<int> vt_faceVertexIndices;
                VtArray<GfVec3f> vt_normals, vt_points;
                VtArray<GfVec2f> vt_uvs;
                VtArray<int> vt_uv_indices, vt_holeIndices;
                TfToken tf_normal_interpolation = TfToken("constant");
                TfToken tf_uv_interpolation = TfToken("constant");
                GfMatrix4d ModelTransform{};

                VtVec3iArray vt_triFaceVertexIndices;
                VtIntArray vt_primitiveParam;
                VtVec3iArray vt_triEdgeIndices;

                // Get Attribute Values
                UsdGeomMesh processGeom(prim);
                UsdAttribute attr_geoHole = processGeom.GetHoleIndicesAttr();
                UsdAttribute attr_faceVertexIndices = prim.GetAttribute(tf_faceVertexIndices);
                UsdAttribute attr_normals = prim.GetAttribute(tf_normals);
                UsdAttribute attr_points = prim.GetAttribute(tf_points);
                UsdAttribute attr_uv = prim.GetAttribute(tf_uv);

                attr_faceVertexIndices.Get(&vt_faceVertexIndices, timeCode);
                attr_normals.Get(&vt_normals, timeCode);
                attr_points.Get(&vt_points, timeCode);
                attr_uv.Get(&vt_uvs, timeCode);
                attr_geoHole.Get(&vt_holeIndices, timeCode);

                // Get normal and uv interpolation
                if (has_normal) {
                    UsdGeomPrimvar primvar_normal(attr_normals);
                    tf_normal_interpolation = primvar_normal.GetInterpolation();
                }

                if (has_uv) {
                    UsdGeomPrimvar primvars_uv(attr_uv);
                    tf_uv_interpolation = primvars_uv.GetInterpolation();
                    primvars_uv.GetIndices(&vt_uv_indices, timeCode);
                }

                UsdGeomXformable xformable(prim);
                ModelTransform = xformable.ComputeLocalToWorldTransform(timeCode);

                // Define a new `faceVertexIndices` array for Triangulation
                VtArray<int> vt_faceVertexIndices_reorder(vt_faceVertexIndices.size());

                myTimer forloopTimer;
                forloopTimer.setStartPoint();
                // int[] faceVertexIndex_index = [0, 1, 2, 3, 4, ...]
                for (int faceVertexIndex_index = 0; faceVertexIndex_index < vt_faceVertexIndices.size(); faceVertexIndex_index++) {
                    // int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, ...]
                    int faceVertexIndex = vt_faceVertexIndices[faceVertexIndex_index];
                    int tempIndex = faceVertexIndex_index + start_pointer;

                    // ----- Point ----- //
                    GfVec3f vt_point = ModelTransform.Transform(vt_points[faceVertexIndex]);
                    vt_gl_position[tempIndex] = vt_point;

                    // ----- Normal ----- //
                    if (has_normal) {
                        int normal_index = 0;
                        if (tf_normal_interpolation == "vertex") {
                            normal_index = faceVertexIndex;
                        } else if (tf_normal_interpolation == "faceVarying") {
                            normal_index = faceVertexIndex_index;
                        }

                        // Set Normal
                        if (vt_normals.empty()) {
                            vt_gl_normal[tempIndex] = GfVec3f(0.0, 1.0, 0.0);
                        } else {
                            GfVec3f vt_normal = vt_normals[normal_index];
                            vt_gl_normal[tempIndex] = vt_normal;
                        }
                    } else {
                        vt_gl_normal[tempIndex] = GfVec3f(0.0, 1.0, 0.0);
                    }

                    // ----- UV ----- //
                    if (has_uv) {
                        int uv_index = 0;
                        if (vt_uv_indices.empty()) {
                            if (tf_uv_interpolation == "vertex") {
                                uv_index = faceVertexIndex;
                            } else if (tf_uv_interpolation == "faceVarying") {
                                uv_index = faceVertexIndex_index;
                            }
                        } else {
                            if (tf_uv_interpolation == "vertex") {
                                uv_index = vt_uv_indices[faceVertexIndex];
                            } else if (tf_uv_interpolation == "faceVarying") {
                                uv_index = vt_uv_indices[faceVertexIndex_index];
                            }
                        }

                        // Set UV
                        if (vt_uvs.empty()) {
                            vt_gl_texCoord[tempIndex] = GfVec2f(0.0, 0.0);
                        } else {
                            GfVec2f vt_uv = vt_uvs[uv_index];
                            vt_gl_texCoord[tempIndex] = vt_uv;
                        }
                    } else {
                        vt_gl_texCoord[tempIndex] = GfVec2f(0.0, 0.0);
                    }

                    // e.g.   int[] faceVertexIndices     = [2, 0, 1, 3, 5, 4, 2, 3]
                    //     -> int[] faceVertexIndices_new = [0, 1, 2, 3, 4, 5, 6, 7]
                    vt_faceVertexIndices_reorder[faceVertexIndex_index] = faceVertexIndex_index;
                }

                forloopTimer.setEndPoint();
                QString info_for_loop = QString("<") + QString(prim.GetPath().GetString().c_str()) + QString("> -----> For loop");

                mtx.lock();
                actually_points += vt_faceVertexIndices.size();
                mtx.unlock();

                subtimer.setEndPoint();
                QString info_before = QString("<") + QString(prim.GetPath().GetString().c_str()) + QString("> -----> Before Compute TriangleIndices");
                subtimer.printDuration(info_before.toStdString().c_str());

                subtimer.setStartPoint();
                if (!m_has_triangulated) {
                    // ----- Simple Triangulation ----- //
                    simpleComputeTriangleIndices(vt_faceVertexCounts, vt_faceVertexIndices_reorder, UsdGeomTokens->rightHanded, vt_triFaceVertexIndices);

                    // ----- Set Vertex indices ----- //
                    // vt_triFaceVertexIndices: [(0, 1, 3), (1, 2, 3), (2, 3, 1), ...}
                    int offsetPtr = (number_of_triangulation - int(vt_triFaceVertexIndices.size())) * 3;
                    for (int i = 0; i < vt_triFaceVertexIndices.size(); i++) {
                        indices[offsetPtr + i * 3 + 0] = vt_triFaceVertexIndices[i][0] + start_pointer;
                        indices[offsetPtr + i * 3 + 1] = vt_triFaceVertexIndices[i][1] + start_pointer;
                        indices[offsetPtr + i * 3 + 2] = vt_triFaceVertexIndices[i][2] + start_pointer;
                    }
                }
                subtimer.setEndPoint();

                QString info_triangulation = QString("<") + QString(prim.GetPath().GetString().c_str()) + QString("> Compute TriangleIndices");
                subtimer.printDuration(info_triangulation.toStdString().c_str());
            });

    vt_gl_position.resize(actually_points);
    vt_gl_texCoord.resize(actually_points);
    vt_gl_normal.resize(actually_points);

    m_has_triangulated = true;
    m_timer.setEndPoint();
    m_timer.printDuration("TraverseAll");

    m_timer.setStartPoint();
    initGeometryDefault();
    m_timer.setEndPoint();
    m_timer.printDuration("InitGeometry buffer allocate");
}

void usdParser::parseMeshWireframe() {
    spdlog::info("----------> Parse Mesh Wireframe");

    myTimer t;
    t.setStartPoint();

    // ----- Method 1 ----- //
    int totalMax = 0;
    int stepMax = 0;

    bool method_default = false;
    if (method_default) {
        // 存在重复画线的问题
        for (UsdPrim prim: stage->TraverseAll()) {
            int left = 0;
            int right = 0;

            if (prim.GetTypeName() == "Mesh") {
                spdlog::info("Mesh Name: {}", prim.GetPath().GetString().c_str());

                // Get Visibility
                UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
                TfToken visibility;
                attr_visibility.Get(&visibility, currenTimeCode);
                if (visibility == UsdGeomTokens->invisible) {
                    continue;
                }

                // ----- Processing line position ----- //
                TfToken tf_points("points");
                VtArray < GfVec3f > vt_points;
                UsdAttribute attr_points = prim.GetAttribute(tf_points);
                attr_points.Get(&vt_points, currenTimeCode);

                UsdGeomXformable xformable(prim);
                GfMatrix4d ModelTransform = xformable.ComputeLocalToWorldTransform(currenTimeCode);

                // ----- Processing line indices ----- //
                VtArray<int> vt_faceVertexCounts, vt_faceVertexIndices;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
                UsdAttribute attr_faceVertexIndices = prim.GetAttribute(TfToken("faceVertexIndices"));

                attr_faceVertexCounts.Get(&vt_faceVertexCounts, currenTimeCode);
                attr_faceVertexIndices.Get(&vt_faceVertexIndices, currenTimeCode);

                for (int &vt_faceVertexCount : vt_faceVertexCounts) {
                    right += vt_faceVertexCount;

                    // process start
                    for (int i = 0; i < vt_faceVertexCount; i++) {
                        int linePtIndex_1 = vt_faceVertexIndices[i + left];
                        int linePtIndex_2 = 0;

                        if (i == vt_faceVertexCount - 1) {
                            linePtIndex_2 = vt_faceVertexIndices[left];
                        } else {
                            linePtIndex_2 = vt_faceVertexIndices[i + left + 1];
                        }

                        stepMax = std::max(std::max(linePtIndex_1, linePtIndex_2), stepMax);

                        if (linePtIndex_1 > linePtIndex_2)
                            std::swap(linePtIndex_1, linePtIndex_2);

                        int p1 = linePtIndex_1 + totalMax;
                        int p2 = linePtIndex_2 + totalMax;
                        QPair<int, int> pair(p1, p2);

                        if (wireframeSet.find(pair) == wireframeSet.end()) {
                            wireframeSet.insert(pair);

                            indices_wireframe.emplace_back(linePtIndex_1 + totalMax);
                            indices_wireframe.emplace_back(linePtIndex_2 + totalMax);

                            vt_gl_position_wireframe.emplace_back(ModelTransform.Transform(vt_points[linePtIndex_1]));
                            vt_gl_position_wireframe.emplace_back(ModelTransform.Transform(vt_points[linePtIndex_2]));
                        }
                    }
                    // process end

                    left = right;
                }
                totalMax += stepMax;
                stepMax = 0;
            }
        }
    } else {
        // ----- Method 2 ----- //
        int totalWireframe = 0, meshIndex = 0, indicesSize = 0;
        std::map<std::string, std::vector<int>> meshDict;

        // --------------------------------------------------
        std::vector<VtVec3fArray> wireframeDataVec;
        // --------------------------------------------------

        for (UsdPrim prim: stage->TraverseAll()) {
            if (prim.GetTypeName() == "Mesh") {
            spdlog::info("Mesh Name: {}", prim.GetPath().GetString().c_str());

                // ----- Get Visibility ----- //
                UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
                TfToken visibility;
                attr_visibility.Get(&visibility, currenTimeCode);
                if (visibility == UsdGeomTokens->invisible) {
                    continue;
                }

                std::vector<int> currentProcessMeshData;

                // ----- Processing line indices ----- //
                VtArray<int> vt_faceVertexCounts;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
                attr_faceVertexCounts.Get(&vt_faceVertexCounts, currenTimeCode);

                currentProcessMeshData.emplace_back(meshIndex);
                currentProcessMeshData.emplace_back(indicesSize);
                for (int &vt_faceVertexCount : vt_faceVertexCounts) {
                    totalWireframe += vt_faceVertexCount;
                    indicesSize += vt_faceVertexCount * 2;
                }
                currentProcessMeshData.emplace_back(indicesSize);
                meshDict[prim.GetPath().GetString()] = currentProcessMeshData;
                meshIndex++;
            }
        }

        spdlog::info("----------> Parallel processing line position");
        tbb::parallel_do(
                stage->TraverseAll(),
                [this, &meshDict, &wireframeDataVec](const UsdPrim& prim) {
                    if (prim.GetTypeName() != "Mesh") {
                        return;
                    } else {
                        UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
                        TfToken visibility;
                        attr_visibility.Get(&visibility, currenTimeCode);
                        if (visibility == UsdGeomTokens->invisible) {
                            return;
                        }
                    }

                    QSet<QPair<int, int>> tempWireframe;
                    VtVec3fArray temp_position_wireframe;

                    // Range: [ start_pointer, end_pointer - 1 ]
                    std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
                    int start_pointer = mesh_data[1];
                    int end_pointer = mesh_data[2];

                    UsdGeomXformable xformable(prim);
                    GfMatrix4d ModelTransform = xformable.ComputeLocalToWorldTransform(this->currenTimeCode);

                    // Get Attribute Values
                    VtArray<int> vt_faceVertexCounts;
                    UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
                    attr_faceVertexCounts.Get(&vt_faceVertexCounts, this->currenTimeCode);

                    VtArray<int> vt_faceVertexIndices;
                    UsdAttribute attr_faceVertexIndices = prim.GetAttribute(TfToken("faceVertexIndices"));
                    attr_faceVertexIndices.Get(&vt_faceVertexIndices, this->currenTimeCode);

                    VtArray<GfVec3f> vt_points;
                    UsdAttribute attr_points = prim.GetAttribute(TfToken("points"));
                    attr_points.Get(&vt_points, currenTimeCode);

                    int left = 0;
                    int right = 0;
                    for (int &vt_faceVertexCount : vt_faceVertexCounts) {
                        if (vt_faceVertexCount > 2) {
                            right += vt_faceVertexCount;

                            // process start
                            for (int i = 0; i < vt_faceVertexCount; i++) {
                                int linePtIndex_1 = vt_faceVertexIndices[i + left];
                                int linePtIndex_2 = 0;

                                if (i == vt_faceVertexCount - 1) {
                                    linePtIndex_2 = vt_faceVertexIndices[left];
                                } else {
                                    linePtIndex_2 = vt_faceVertexIndices[i + left + 1];
                                }

                                if (linePtIndex_1 > linePtIndex_2)
                                    std::swap(linePtIndex_1, linePtIndex_2);

                                int p1 = linePtIndex_1;
                                int p2 = linePtIndex_2;
                                QPair<int, int> pair(p1, p2);

                                if (tempWireframe.find(pair) == tempWireframe.end()) {
                                    tempWireframe.insert(pair);

                                    temp_position_wireframe.emplace_back(ModelTransform.Transform(vt_points[linePtIndex_1]));
                                    temp_position_wireframe.emplace_back(ModelTransform.Transform(vt_points[linePtIndex_2]));
                                }
                            }
                            // process end

                            left = right;
                        }
                    }

                    mtx.lock();
                    wireframeDataVec.push_back(temp_position_wireframe);
                    mtx.unlock();
                });

        for (auto & data : wireframeDataVec) {
            for (auto & d : data) {
                vt_gl_position_wireframe.emplace_back(d);
            }
        }
    }

    // ----- Set buffer data ----- //
    initGeometryBufferWireframe();
    initGeometryWireframeMapRange();
    t.setEndPoint();
    t.printDuration("Mesh Wireframe");
}

void usdParser::initGeometryBufferWireframe() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoWireframe);

    vboWireframe.bind();
    vboWireframe.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboWireframe.allocate(nullptr, vt_gl_position_wireframe.size() * sizeof(GfVec3f));
}

void usdParser::initGeometryWireframeMapRange() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoWireframe);

    vboWireframe.bind();
    auto posPtr = reinterpret_cast<GfVec3f*>(vboWireframe.mapRange(0, vt_gl_position_wireframe.size() * sizeof(GfVec3f), QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite));

    std::copy(vt_gl_position_wireframe.begin(), vt_gl_position_wireframe.end(), posPtr);
    vboWireframe.unmap();
    vboWireframe.release();
}

void usdParser::initGeometryDefault() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoGeometry);

    vbos[0].bind();
    vbos[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[0].allocate(vt_gl_position.data(), vt_gl_position.size() * sizeof(GfVec3f));

    vbos[1].bind();
    vbos[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[1].allocate(vt_gl_texCoord.data(), vt_gl_texCoord.size() * sizeof(GfVec2f));

    vbos[2].bind();
    vbos[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[2].allocate(vt_gl_normal.data(), vt_gl_normal.size() * sizeof(GfVec3f));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(indices.data(), indices.size() * sizeof(GLuint));
}

void usdParser::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoGeometry);
    int vboIndex = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    vbos[vboIndex].bind();
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(GfVec3f));
    vboIndex++;

    vbos[vboIndex].bind();
    int coordLocation = program->attributeLocation("aCoord");
    program->enableAttributeArray(coordLocation);
    program->setAttributeBuffer(coordLocation, GL_FLOAT, 0, 2, sizeof(GfVec2f));
    vboIndex++;

    vbos[vboIndex].bind();
    int normalLocation = program->attributeLocation("aNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, 0, 3, sizeof(GfVec3f));
    vboIndex++;
}

void usdParser::setupAttributePointerWireframe(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoWireframe);

    vboWireframe.bind();
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(GfVec3f));
}

void usdParser::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoGeometry);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)nullptr);
}

void usdParser::drawGeometryWireframe(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vaoWireframe);
    glDrawArrays(GL_LINES, 0, vt_gl_position_wireframe.size());

//    GLint first[4] = {0, 5, 10, 15};
//    GLint count[4] = {5, 5, 5, 5};
//    glMultiDrawArrays(GL_LINE_STRIP, first, count, 4);
}

bool usdParser::fanTriangulate(GfVec3i &dst, VtArray<int> const &src, int offset, int index, int size, bool flip) {
    if (offset + index + 2 >= size) {
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
        return false;
    }

    if (flip) {
        dst[0] = src[offset];
        dst[1] = src[offset + index + 2];
        dst[2] = src[offset + index + 1];
    } else {
        dst[0] = src[offset];
        dst[1] = src[offset + index + 1];
        dst[2] = src[offset + index + 2];
    }

    return true;
}

bool usdParser::simpleComputeTriangleIndices(VtArray<int> &faceVertexCounts, VtArray<int> &faceVertexIndices, const TfToken& orientation, VtVec3iArray &tri_indices) {
    int numFaces = faceVertexCounts.size();
    int numVertIndices = faceVertexIndices.size();
    int numTris = 0;

    bool invalidTopology = false;

    for (int i = 0; i < numFaces; ++i) {
        int nv = faceVertexCounts[i] - 2;
        if (nv < 1) {
            invalidTopology = true;
            spdlog::warn("InvalidTopology face index: {}", i);
            return invalidTopology;
        } else {
            numTris += nv;
        }
    }

    tri_indices.resize(numTris);
    bool flip = (orientation != pxr::UsdGeomTokens->rightHanded);

    for (int i = 0, tv = 0, v = 0; i < numFaces; ++i) {
        int nv = faceVertexCounts[i];
        if (nv < 3) {
        } else {
            for (int j = 0; j < nv-2; ++j) {
                if (!fanTriangulate(tri_indices[tv], faceVertexIndices, v, j, numVertIndices, flip)) {
                    invalidTopology = true;
                    return invalidTopology;
                }
                ++tv;
            }
        }
        v += nv;
    }

    return invalidTopology;
}
