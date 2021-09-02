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
    vao.create();
    for (int i=0; i<attributeCount; ++i) {
        QOpenGLBuffer vbo;
        vbo.create();
        vbos.push_back(vbo);
    }
    ebo.create();
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
/*
void usdParser::getDataBySpecifyFrame_default(UsdTimeCode timeCode) {
    spdlog::info("\n\tGet data by specify frame: {}", timeCode.GetValue());

    currenTimeCode = timeCode;

    myTimer m_timer;
    m_timer.setStartPoint();
    if (!m_has_triangulated)
        m_indicesCount = 0;

    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() == "Mesh") {
            spdlog::info("\n\t Processing mesh: {}", prim.GetName().GetString());

            // Mesh Attribute Token
            TfToken tf_faceVertexCounts("faceVertexCounts");
            TfToken tf_faceVertexIndices("faceVertexIndices");
            TfToken tf_normals("normals");
            TfToken tf_points("points");

            // Get UV Attribute Token
            bool has_uv;
            TfToken tf_uv;
            getUVToken(prim, tf_uv, has_uv);

            // Check Normal Attribute
            bool has_normal = prim.HasProperty(tf_normals);

            // USD Attributes
            VtArray<int> vt_faceVertexCounts, vt_faceVertexIndices;
            VtArray<GfVec3f> vt_normals, vt_points;
            VtArray<GfVec2f> vt_uvs;
            VtArray<int> vt_uv_indices, vt_holeIndices;
            TfToken tf_normal_interpolation = TfToken("constant");
            TfToken tf_uv_interpolation = TfToken("constant");
            GfMatrix4d ModelTransform{};

            VtVec3iArray vt_triFaceVertexIndices;
            VtIntArray vt_primitiveParam;
            VtVec3iArray vt_triEdgeIndices;
            //VtIntArray vt_triEdgeIndices;  // for newly usd version

            // Get Attribute Values
            UsdGeomMesh processGeom(prim);
            UsdAttribute attr_geoHole = processGeom.GetHoleIndicesAttr();
            UsdAttribute attr_faceVertexCounts = prim.GetAttribute(tf_faceVertexCounts);
            UsdAttribute attr_faceVertexIndices = prim.GetAttribute(tf_faceVertexIndices);
            UsdAttribute attr_normals = prim.GetAttribute(tf_normals);
            UsdAttribute attr_points = prim.GetAttribute(tf_points);
            UsdAttribute attr_uv = prim.GetAttribute(tf_uv);

            attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);
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

            // int[] faceVertexIndex_index = [0, 1, 2, 3, 4, ...]
            for (int faceVertexIndex_index = 0; faceVertexIndex_index < vt_faceVertexIndices.size(); faceVertexIndex_index++) {
                // int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, ...]
                int faceVertexIndex = vt_faceVertexIndices[faceVertexIndex_index];

                // ----- Point ----- //
                GfVec3f vt_point = ModelTransform.Transform(vt_points[faceVertexIndex]);
                vt_gl_position.emplace_back(vt_point);

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
                        vt_gl_normal.emplace_back(GfVec3f(0.0, 1.0, 0.0));
                    } else {
                        GfVec3f vt_normal = vt_normals[normal_index];
                        vt_gl_normal.emplace_back(vt_normal);
                    }
                } else {
                    vt_gl_normal.emplace_back(GfVec3f(0.0, 1.0, 0.0));
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
                        vt_gl_texCoord.emplace_back(GfVec2f(0.0, 0.0));
                    } else {
                        GfVec2f vt_uv = vt_uvs[uv_index];
                        vt_gl_texCoord.emplace_back(vt_uv);
                    }
                } else {
                    vt_gl_texCoord.emplace_back(GfVec2f(0.0, 0.0));
                }

                vt_faceVertexIndices_reorder[faceVertexIndex_index] = faceVertexIndex_index;
            }

            // Compute triangulation
            myTimer subtimer;
            subtimer.setStartPoint();
            if (!m_has_triangulated) {
                // ----- Triangulation ----- //
                HdMeshTopology topology(UsdGeomTokens->none, UsdGeomTokens->rightHanded,
                                        vt_faceVertexCounts, vt_faceVertexIndices_reorder, vt_holeIndices);
                HdMeshUtil mesh_util(&topology, prim.GetPath());
                mesh_util.ComputeTriangleIndices(&vt_triFaceVertexIndices, &vt_primitiveParam, &vt_triEdgeIndices);

                // vt_triFaceVertexIndices: [(0, 1, 3), (1, 2, 3), (2, 3, 1), ...]
                indices.resize(m_indicesCount + vt_triFaceVertexIndices.size() * 3);
                for (int i = 0; i < vt_triFaceVertexIndices.size(); i++) {
                    indices[i * 3 + m_indicesCount + 0] = m_indexIncrease + vt_triFaceVertexIndices[i][0];
                    indices[i * 3 + m_indicesCount + 1] = m_indexIncrease + vt_triFaceVertexIndices[i][1];
                    indices[i * 3 + m_indicesCount + 2] = m_indexIncrease + vt_triFaceVertexIndices[i][2];
                }
                m_indicesCount += vt_triFaceVertexIndices.size() * 3;
            }
            subtimer.setEndPoint();
            QString info = QString("Compute TriangleIndices <") + QString(prim.GetName().GetString().c_str()) + QString(">");
            subtimer.printDuration(info.toStdString().c_str());

            // record vertex data amount for each Prim
            m_indexIncrease += vt_faceVertexIndices_reorder.size();
        }
    }

    m_has_triangulated = true;
    m_timer.setEndPoint();
    m_timer.printDuration("TraverseAll");

    m_timer.setStartPoint();
    initGeometry();
    m_timer.setEndPoint();
    m_timer.printDuration("InitGeometry buffer allocate");
}
*/
void usdParser::getDataBySpecifyFrame_TBB(UsdTimeCode timeCode) {
    auto search = geometry_data.find(timeCode.GetValue());
    if(search != geometry_data.end()){
//        qDebug() << "this timeCode " << timeCode.GetValue() << " is already in the geometry_data";
        return;
    }
    if(timeCode.GetValue() < stage->GetStartTimeCode() || timeCode.GetValue() > stage->GetEndTimeCode()){
//        qDebug() << "this timeCode "
//                 << timeCode.GetValue() << " is not a valid timeCode, the range must be from"
//                 << stage->GetStartTimeCode()
//                 << "to"<< stage->GetEndTimeCode();
        return;
    }

    spdlog::info("\n\tGet data by specify frame: {}", timeCode.GetValue());

    currentTimeCode = timeCode;

    myTimer m_timer;
    m_timer.setStartPoint();
//    if (!m_has_triangulated)
    m_indicesCount = 0;

    // ----- Mesh dictionary ----- //
    // { "mesh1":{0, 0, 4, 2}, "mesh2":{1, 4, 28, 14}, ... }
    // { "mesh_name": {mesh_index, indices_start, indices_end, the_number_of_triangles_accumulated_at_the_current_position}, ... }
    std::map<std::string, std::vector<int>> meshDict;

    // ----- Precalculate the number of triangulation and mesh index ----- //
    int totalTriangulation = 0, meshIndex = 0, index_pointer = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() == "Mesh") {
            // Get Visibility
            UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
            TfToken visibility;
            attr_visibility.Get(&visibility, currentTimeCode);
            if (visibility == UsdGeomTokens->invisible) {
                continue;
            }

            std::vector<int> currentProcessMeshData;
            VtArray<int> vt_faceVertexCounts, vt_faceVertexIndices;

            UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
            UsdAttribute attr_faceVertexIndices = prim.GetAttribute(TfToken("faceVertexIndices"));

            attr_faceVertexCounts.Get(&vt_faceVertexCounts, currentTimeCode);
            attr_faceVertexIndices.Get(&vt_faceVertexIndices, currentTimeCode);

            currentProcessMeshData.emplace_back(meshIndex);
            currentProcessMeshData.emplace_back(index_pointer); // represent start pointer
            for (int vt_faceVertexCount : vt_faceVertexCounts) {
                totalTriangulation += vt_faceVertexCount - 2;
            }
            index_pointer += int(vt_faceVertexIndices.size());
            currentProcessMeshData.emplace_back(index_pointer); // represent end pointer
            currentProcessMeshData.emplace_back(totalTriangulation);

            meshDict[prim.GetName().GetString()] = currentProcessMeshData;
            meshIndex++;
        }
    }

    // ----- Preallocate memory ----- //
    geometry_data[currentTimeCode.GetValue()] = VertexData();
    VertexData &vertex_data = geometry_data[timeCode.GetValue()];
    vertex_data.indices.resize(totalTriangulation * 3); // 3 points per triangulation
    vertex_data.vt_gl_position.resize(index_pointer);
    vertex_data.vt_gl_texCoord.resize(index_pointer);
    vertex_data.vt_gl_normal.resize(index_pointer);

    // ----- Actually the number of points, uv, normal ----- //
    int actually_points = 0, actually_uv = 0, actually_normal = 0;

    // ----- Parallel traverse all prim ----- //
    tbb::parallel_do(
            stage->TraverseAll(),
            [this, &timeCode, &actually_points, &meshDict, &vertex_data ](UsdPrim prim) {
                if (prim.GetTypeName() != "Mesh") {
                    return;
                } else {
                    UsdAttribute attr_visibility = prim.GetAttribute(TfToken("visibility"));
                    TfToken visibility;
                    attr_visibility.Get(&visibility, currentTimeCode);
                    if (visibility == UsdGeomTokens->invisible) {
                        return;
                    }
                }

                myTimer subtimer;
                subtimer.setStartPoint();

                VtArray<int> vt_faceVertexCounts;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
                attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

                std::vector<int> mesh_data = meshDict[prim.GetName().GetString()];
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
                //VtIntArray vt_triEdgeIndices;

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

                tbb::parallel_for(0, int(vt_faceVertexIndices.size()), 1,
                                  [this, &vt_faceVertexIndices, &ModelTransform, &start_pointer, &vt_faceVertexIndices_reorder,
                                          vt_points, vt_normals, vt_uv_indices, vt_uvs, &has_normal, &tf_normal_interpolation, &has_uv, &tf_uv_interpolation, &vertex_data](int faceVertexIndex_index) {

                // int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, ...]
                int faceVertexIndex = vt_faceVertexIndices[faceVertexIndex_index];

                // ----- Point ----- //
                GfVec3f vt_point = ModelTransform.Transform(vt_points[faceVertexIndex]);
                vertex_data.vt_gl_position[faceVertexIndex_index + start_pointer] = vt_point;

                // e.g.   int[] faceVertexIndices     = [2, 0, 1, 3, 5, 4, 2, 3]
                //     -> int[] faceVertexIndices_new = [0, 1, 2, 3, 4, 5, 6, 7]
                vt_faceVertexIndices_reorder[faceVertexIndex_index] = faceVertexIndex_index;

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
                        vertex_data.vt_gl_normal[faceVertexIndex_index + start_pointer] = GfVec3f(0.0, 1.0, 0.0);
                    } else {
                        GfVec3f vt_normal = vt_normals[normal_index];
                        vertex_data.vt_gl_normal[faceVertexIndex_index + start_pointer] = vt_normal;
                    }
                } else {
                    vertex_data.vt_gl_normal[faceVertexIndex_index + start_pointer] = GfVec3f(0.0, 1.0, 0.0);
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
                        vertex_data.vt_gl_texCoord[faceVertexIndex_index + start_pointer] = GfVec2f(0.0, 0.0);
                    } else {
                        GfVec2f vt_uv = vt_uvs[uv_index];
                        vertex_data.vt_gl_texCoord[faceVertexIndex_index + start_pointer] = vt_uv;
                    }
                } else {
                    vertex_data.vt_gl_texCoord[faceVertexIndex_index + start_pointer] = GfVec2f(0.0, 0.0);
                }
            });

                mtx.lock();
                actually_points += vt_faceVertexIndices.size();
                mtx.unlock();

                subtimer.setEndPoint();
                QString info_before = QString("<") + QString(prim.GetName().GetString().c_str()) + QString("> -----> Before Compute TriangleIndices");
                subtimer.printDuration(info_before.toStdString().c_str());

                subtimer.setStartPoint();
                if (!m_has_triangulated) {
                    // ----- Simple Triangulation ----- //
                    simpleComputeTriangleIndices(vt_faceVertexCounts, vt_faceVertexIndices_reorder, UsdGeomTokens->rightHanded, vt_triFaceVertexIndices);

                    // ----- Set Vertex indices ----- //
                    // vt_triFaceVertexIndices: [(0, 1, 3), (1, 2, 3), (2, 3, 1), ...}
                    int offsetPtr = (number_of_triangulation - int(vt_triFaceVertexIndices.size())) * 3;
                    for (int i = 0; i < vt_triFaceVertexIndices.size(); i++) {
                        vertex_data.indices[offsetPtr + i * 3 + 0] = vt_triFaceVertexIndices[i][0] + start_pointer;
                        vertex_data.indices[offsetPtr + i * 3 + 1] = vt_triFaceVertexIndices[i][1] + start_pointer;
                        vertex_data.indices[offsetPtr + i * 3 + 2] = vt_triFaceVertexIndices[i][2] + start_pointer;
                    }
                }
                subtimer.setEndPoint();

                QString info_triangulation = QString("<") + QString(prim.GetName().GetString().c_str()) + QString("> Compute TriangleIndices ");
                subtimer.printDuration(info_triangulation.toStdString().c_str());
            });

    vertex_data.vt_gl_position.resize(actually_points);
    vertex_data.vt_gl_texCoord.resize(actually_points);
    vertex_data.vt_gl_normal.resize(actually_points);
    qDebug() << "indices size: " << vertex_data.indices.size();

//    m_has_triangulated = true;
    m_timer.setEndPoint();
    m_timer.printDuration("TraverseAll");

    m_timer.setStartPoint();
    initGeometry();
    m_timer.setEndPoint();
    m_timer.printDuration("InitGeometry buffer allocate");
}

void usdParser::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];
    qDebug() << "indices size: " << vertex_data.indices.size();

    vbos[0].bind();
    vbos[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[0].allocate(vertex_data.vt_gl_position.data(), vertex_data.vt_gl_position.size() * sizeof(GfVec3f));

    vbos[1].bind();
    vbos[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[1].allocate(vertex_data.vt_gl_texCoord.data(), vertex_data.vt_gl_texCoord.size() * sizeof(GfVec2f));

    vbos[2].bind();
    vbos[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[2].allocate(vertex_data.vt_gl_normal.data(), vertex_data.vt_gl_normal.size() * sizeof(GfVec3f));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(vertex_data.indices.data(), vertex_data.indices.size() * sizeof(GLuint));
}

void usdParser::setupAttributePointer(QOpenGLShaderProgram *program) {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
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

void usdParser::drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection) {
    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];

    program->bind();

    program->setUniformValue("model", model);
    program->setUniformValue("view", view);
    program->setUniformValue("projection", projection);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    glDrawElements(GL_TRIANGLES, vertex_data.indices.size(), GL_UNSIGNED_INT, (void*)nullptr);
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

void usdParser::updateVertex() {
    getDataBySpecifyFrame_TBB(UsdTimeCode(currentTimeCode.GetValue() + 1.0));

    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];
    vbos[0].bind();
    vbos[0].write(0, vertex_data.vt_gl_position.data(), vertex_data.vt_gl_position.size()* sizeof(GfVec3f));
    vbos[0].release();

    vbos[1].bind();
    vbos[1].write(0, vertex_data.vt_gl_texCoord.data(), vertex_data.vt_gl_texCoord.size()* sizeof(GfVec2f));
    vbos[1].release();

    vbos[2].bind();
    vbos[2].write(0, vertex_data.vt_gl_normal.data(), vertex_data.vt_gl_normal.size()* sizeof(GfVec3f));
    vbos[2].release();
}
