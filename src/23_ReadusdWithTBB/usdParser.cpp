#include "usdParser.h"

usdParser::usdParser(QString &path) : usdFilePath(path), ebo(QOpenGLBuffer::IndexBuffer) {
    stage = UsdStage::Open(usdFilePath.toStdString());

    // Base parameter
    animStartFrame = stage->GetStartTimeCode();
    currentTimeCode = animStartFrame;
    animEndFrame = stage->GetEndTimeCode();
    lastDrewTimeCode = UsdTimeCode(LDBL_MAX);  // init the value by max to avoid same value determined

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

void usdParser::getDataBySpecifyFrame_default(UsdTimeCode timeCode) {
    spdlog::info("\tGet data by specify frame: {}", timeCode.GetValue());

    currentTimeCode = timeCode;

    myTimer m_timer;
    m_timer.setStartPoint("TraverseAll");

    int __indices_count = 0;
    int __index_increase = 0;

    geometry_data[timeCode.GetValue()] = VertexData();
    VertexData &vertex_data = geometry_data[timeCode.GetValue()];

    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() == "Mesh") {
            spdlog::info("\n\t Processing mesh: {}", prim.GetPath().GetString());

            // Mesh Attribute Token
            // Get UV Attribute Token
            bool has_uv;
            TfToken tf_uv;
            getUVToken(prim, tf_uv, has_uv);

            // Check Normal Attribute
            bool has_normal = prim.HasProperty(UsdGeomTokens->normals);

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
            UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
            UsdAttribute attr_faceVertexIndices = prim.GetAttribute(UsdGeomTokens->faceVertexIndices);
            UsdAttribute attr_normals = prim.GetAttribute(UsdGeomTokens->normals);
            UsdAttribute attr_points = prim.GetAttribute(UsdGeomTokens->points);
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
                vertex_data.vt_gl_position.emplace_back(vt_point);

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
                        vertex_data.vt_gl_normal.emplace_back(GfVec3f(0.0, 1.0, 0.0));
                    } else {
                        GfVec3f vt_normal = vt_normals[normal_index];
                        vertex_data.vt_gl_normal.emplace_back(vt_normal);
                    }
                } else {
                    vertex_data.vt_gl_normal.emplace_back(GfVec3f(0.0, 1.0, 0.0));
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
                        vertex_data.vt_gl_texCoord.emplace_back(GfVec2f(0.0, 0.0));
                    } else {
                        GfVec2f vt_uv = vt_uvs[uv_index];
                        vertex_data.vt_gl_texCoord.emplace_back(vt_uv);
                    }
                } else {
                    vertex_data.vt_gl_texCoord.emplace_back(GfVec2f(0.0, 0.0));
                }

                vt_faceVertexIndices_reorder[faceVertexIndex_index] = faceVertexIndex_index;
            }

            // Compute triangulation
            myTimer subtimer;
            subtimer.setStartPoint("Compute TriangleIndices");

            // ----- Triangulation ----- //
            HdMeshTopology topology(UsdGeomTokens->none, UsdGeomTokens->rightHanded,
                                    vt_faceVertexCounts, vt_faceVertexIndices_reorder, vt_holeIndices);
            HdMeshUtil mesh_util(&topology, prim.GetPath());
            mesh_util.ComputeTriangleIndices(&vt_triFaceVertexIndices, &vt_primitiveParam, &vt_triEdgeIndices);

            // vt_triFaceVertexIndices: [(0, 1, 3), (1, 2, 3), (2, 3, 1), ...]
            vertex_data.indices.resize(__indices_count + vt_triFaceVertexIndices.size() * 3);
            for (int i = 0; i < vt_triFaceVertexIndices.size(); i++) {
                vertex_data.indices[i * 3 + __indices_count + 0] = __index_increase + vt_triFaceVertexIndices[i][0];
                vertex_data.indices[i * 3 + __indices_count + 1] = __index_increase + vt_triFaceVertexIndices[i][1];
                vertex_data.indices[i * 3 + __indices_count + 2] = __index_increase + vt_triFaceVertexIndices[i][2];
            }
            __indices_count += vt_triFaceVertexIndices.size() * 3;

//            computeTriangleIndices(vt_faceVertexCounts, vertex_data, start_tri_pointer * 3, UsdGeomTokens->rightHanded);

            QString info_triangulation = QString("Compute TriangleIndices <") + QString(prim.GetPath().GetString().c_str()) + QString(">");
            subtimer.setEndPoint(info_triangulation.toStdString());

            // record vertex data amount for each Prim
            __index_increase += vt_faceVertexIndices.size();
        }

    }


    m_timer.setEndPoint();

//    m_timer.setStartPoint("InitGeometry buffer allocate");
//    initGeometry();
}

void usdParser::getDataBySpecifyFrame_optimized_non_TBB(UsdTimeCode timeCode) {

    auto search = geometry_data.find(timeCode.GetValue());

    if(search != geometry_data.end()){
        //qDebug() << "this timeCode " << timeCode.GetValue() << " is already in the geometry_data";
        return;
    }
    if(timeCode < stage->GetStartTimeCode() || timeCode > stage->GetEndTimeCode()){
        //qDebug() << "this is not a valid timeCode " << timeCode.GetValue();
        return;
    }

    spdlog::info("\tGet data by specify frame: {}", timeCode.GetValue());

    myTimer m_timer;
    m_timer.setStartPoint("TraverseAll");
    m_timer.setStartPoint("TraverseFaceVertexCounts");

    // ----- Mesh dictionary ----- //
    // { "mesh1":{0, 0, 4, 2}, "mesh2":{1, 4, 28, 14}, ... }
    // { "mesh_name": {mesh_index, indices_start, indices_end, the_number_of_triangles_accumulated_at_the_current_position}, ... }
    std::map<std::string, std::vector<int>> meshDict;

    // ----- Precalculate the number of triangulation and mesh index ----- //
    int totalTriangulation = 0, meshIndex = 0, index_pointer = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() != "Mesh") { continue; }

        // Get Visibility
        UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
        TfToken visibility;
        attr_visibility.Get(&visibility, timeCode);

        if (visibility == UsdGeomTokens->invisible) { continue; }

        std::vector<int> currentProcessMeshData(5);
        VtArray<int> vt_faceVertexCounts;

        UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);

        attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

        currentProcessMeshData[0] = meshIndex;
        currentProcessMeshData[1] = index_pointer; // represent start pointer
        currentProcessMeshData[2] = totalTriangulation; // represent start pointer for triangle indices
        for (int vt_faceVertexCount : vt_faceVertexCounts) {
            totalTriangulation += vt_faceVertexCount - 2;
            index_pointer += vt_faceVertexCount;
        }
        currentProcessMeshData[3] = index_pointer; // represent end pointer
        currentProcessMeshData[4] = totalTriangulation;

        meshDict[prim.GetPath().GetString()] = currentProcessMeshData;
        meshIndex++;
    }
    m_timer.setEndPoint();

    // ----- Preallocate memory ----- //
    geometry_data[timeCode.GetValue()] = VertexData();
    VertexData &vertex_data = geometry_data[timeCode.GetValue()];
    vertex_data.indices.resize(totalTriangulation * 3); // 3 points per triangulation
    vertex_data.vt_gl_position.resize(index_pointer);
    vertex_data.vt_gl_texCoord.resize(index_pointer);
    vertex_data.vt_gl_normal.resize(index_pointer);
    vertex_data.vt_gl_display_color.resize(index_pointer);

    // ----- Actually the number of points, uv, normal ----- //
    int actually_points = 0, actually_uv = 0, actually_normal = 0;

    // ----- Parallel traverse all prim ----- //
//    tbb::parallel_do(
//            stage->TraverseAll(),
//            [this, &timeCode, &actually_points, &meshDict, &vertex_data ](UsdPrim prim) {
//            }); // tbb::parallel_do  stage->TraverseAll(),

    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() != "Mesh") {
            continue;
        } else {
            UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
            TfToken visibility;
            attr_visibility.Get(&visibility, timeCode);
            if (visibility == UsdGeomTokens->invisible) {
                continue;
            }
        }

        myTimer subtimer;
        subtimer.setStartPoint("Deal with Properties");

        VtArray<int> vt_faceVertexCounts;
        UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
        attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

        std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
        // Range: [ start_pointer, end_pointer - 1 ]
        int mesh_index = mesh_data[0]; //
        int start_pointer = mesh_data[1]; // faceVertexIndices start
        int start_tri_pointer = mesh_data[2]; // triangle amount start index
        int end_pointer = mesh_data[3]; // except this value, end_pointer - 1;
        int number_of_triangulation = mesh_data[4]; // triangle amount

        // Get UV Attribute Token
        bool has_uv;
        TfToken tf_uv;
        this->getUVToken(prim, tf_uv, has_uv);

        // Check Normal Attribute
        bool has_normal = prim.HasProperty(UsdGeomTokens->normals);

        // USD Attributes
        VtArray<int> vt_faceVertexIndices;
        VtArray < GfVec3f > vt_normals, vt_points, vt_displayColor;
        VtArray < GfVec2f > vt_uvs;
        VtArray<int> vt_uv_indices, vt_display_color_indices, vt_holeIndices;
        GfMatrix4d ModelTransform{};
        TfToken vt_orientation;

        // Get Attribute Values
        UsdGeomMesh processGeom(prim);
        UsdAttribute attr_geoHole = processGeom.GetHoleIndicesAttr();
        UsdAttribute attr_faceVertexIndices = prim.GetAttribute(UsdGeomTokens->faceVertexIndices);
        UsdAttribute attr_normals = prim.GetAttribute(UsdGeomTokens->normals);
        UsdAttribute attr_points = prim.GetAttribute(UsdGeomTokens->points);
        UsdAttribute attr_uv = prim.GetAttribute(tf_uv);
        UsdAttribute attr_displayColor = prim.GetAttribute(UsdGeomTokens->primvarsDisplayColor);
        UsdAttribute attr_orientation = prim.GetAttribute(UsdGeomTokens->orientation);

        attr_faceVertexIndices.Get(&vt_faceVertexIndices, timeCode);
        attr_normals.Get(&vt_normals, timeCode);
        attr_points.Get(&vt_points, timeCode);
        attr_uv.Get(&vt_uvs, timeCode);
        attr_displayColor.Get(&vt_displayColor, timeCode);
        attr_geoHole.Get(&vt_holeIndices, timeCode);
        attr_orientation.Get(&vt_orientation, timeCode);

        // Get interpolation
        // normal
        UsdGeomPrimvar primvar_normal(attr_normals);
        TfToken tf_normal_interpolation = primvar_normal.GetInterpolation();
        // uv
        UsdGeomPrimvar primvars_uv(attr_uv);
        TfToken tf_uv_interpolation = primvars_uv.GetInterpolation();
        primvars_uv.GetIndices(&vt_uv_indices, timeCode);
        // displayColor
        UsdGeomPrimvar primvar_displayColor(attr_displayColor);
        TfToken tf_displayColor_interpolation = primvar_displayColor.GetInterpolation();
        primvar_displayColor.GetIndices(&vt_display_color_indices, timeCode);

        // Define xformable for the final position calculation of each point
        UsdGeomXformable xformable(prim);
        ModelTransform = xformable.ComputeLocalToWorldTransform(timeCode);


        // Get Values
        int faceVertexCounts_size = vt_faceVertexCounts.size();
        int face_vertex_index_start = 0;

        // vt_faceVertexCounts -> int[] faceVertexCounts = [4, 4, 4, 4]
        // face_vertex_count_index: 0->1->2->3->...
        for (int face_vertex_count_index = 0;
             face_vertex_count_index < faceVertexCounts_size; ++face_vertex_count_index) {

            // face_vertex_count: 4->4->4->4->...
            int face_vertex_count = vt_faceVertexCounts[face_vertex_count_index];

            // vt_faceVertexIndices -> int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, 5, 4, 3, 4, 7, 6, 4, 5, 8, 7]
            // face_vertex_index: 0->1->2->3->4->5->6->7->...
            for (int face_vertex_index = face_vertex_index_start;
                 face_vertex_index < face_vertex_index_start + face_vertex_count; ++face_vertex_index) {

                // face_vertex: 0->1->4->3->...
                int face_vertex = vt_faceVertexIndices[face_vertex_index];

                // ----- Point ----- //
                vertex_data.vt_gl_position[face_vertex_index + start_pointer] = ModelTransform.Transform(
                        vt_points[face_vertex]);


                // ----- Normal ----- //
                if (vt_normals.empty()) {
                    // Set Normal
                    vertex_data.vt_gl_normal[face_vertex_index + start_pointer] = GfVec3f(0.0, 1.0, 0.0);
                } else {
                    int normal_index = 0;
                    // general
                    if (tf_normal_interpolation == "vertex") {
                        normal_index = face_vertex;
                    } else if (tf_normal_interpolation == "faceVarying") {
                        normal_index = face_vertex_index;
                    }

                    // Set Normal
                    vertex_data.vt_gl_normal[face_vertex_index + start_pointer] = vt_normals[normal_index];
                }


                // ----- UV ----- //
                if (vt_uvs.empty()) {
                    // Set UV
                    // vertex_data.uv[face_vertex_index + start_pointer] = GfVec2f(0.0, 0.0);
                } else {
                    int uv_index = 0;
                    // houdini
                    if (vt_uv_indices.empty()) {
                        if (tf_uv_interpolation == "vertex") {
                            uv_index = face_vertex;
                        } else if (tf_uv_interpolation == "faceVarying") {
                            uv_index = face_vertex_index;
                        }
                    }
                        // maya
                    else {
                        if (tf_uv_interpolation == "vertex") {
                            uv_index = vt_uv_indices[face_vertex];
                        } else if (tf_uv_interpolation == "faceVarying") {
                            uv_index = vt_uv_indices[face_vertex_index];
                        }
                    }
                    // Set UV
                    vertex_data.vt_gl_texCoord[face_vertex_index + start_pointer] = vt_uvs[uv_index];
                }


                // ----- DisplayColor ----- //
                if (vt_displayColor.empty()) {
                    // Set Color
                    vertex_data.vt_gl_display_color[face_vertex_index + start_pointer] = GfVec3f(0.5, 0.5, 0.5);
                } else {
                    int display_color_index = 0;    // <- if "constant"
                    // houdini
                    if (vt_display_color_indices.empty()) {
                        if (tf_displayColor_interpolation == "vertex") {
                            display_color_index = face_vertex;
                        } else if (tf_displayColor_interpolation == "faceVarying") {
                            display_color_index = face_vertex_index;
                        } else if (tf_displayColor_interpolation == "uniform") {
                            display_color_index = face_vertex_count_index;
                        }
                    }
                        // maya
                    else {
                        // if (tf_uv_interpolation == "vertex"), do not deal with it now
                        if (tf_displayColor_interpolation == "uniform") {
                            display_color_index = vt_display_color_indices[face_vertex_count_index];
                        }
                    }

                    // Set Color
                    vertex_data.vt_gl_display_color[face_vertex_index +
                                                    start_pointer] = vt_displayColor[display_color_index];
                }


            } // for loop for vt_faceVertexIndices

            // face_vertex_index_start: 0->4->8->12->...
            face_vertex_index_start += face_vertex_count;

        } // for loop for vt_faceVertexCounts


        subtimer.setEndPoint();

//                spdlog::info("start_tri_pointer - {} - {}", start_tri_pointer, prim.GetPath().GetString());

        subtimer.setStartPoint("Compute TriangleIndices");
        // ----- Simple Triangulation ----- //
        computeTriangleIndices(vt_faceVertexCounts, vertex_data, start_tri_pointer * 3, start_pointer,
                               UsdGeomTokens->rightHanded);

//                QString info_triangulation = QString("Compute TriangleIndices <") + QString(prim.GetPath().GetString().c_str()) + QString(">");
        subtimer.setEndPoint(prim.GetPath().GetString());
    }
    m_timer.setEndPoint();

}

void usdParser::getDataBySpecifyFrame_TBB(UsdTimeCode timeCode) {
    auto search = geometry_data.find(timeCode.GetValue());
    if(search != geometry_data.end()){
        //qDebug() << "this timeCode " << timeCode.GetValue() << " is already in the geometry_data";
        return;
    }
    if(timeCode < stage->GetStartTimeCode() || timeCode > stage->GetEndTimeCode()){
        //qDebug() << "this is not a valid timeCode " << timeCode.GetValue();
        return;
    }

    spdlog::info("\tGet data by specify frame: {}", timeCode.GetValue());

    myTimer m_timer;
    m_timer.setStartPoint("TraverseAll");

    // ----- Mesh dictionary ----- //
    // { "mesh1":{0, 0, 4, 2}, "mesh2":{1, 4, 28, 14}, ... }
    // { "mesh_name": {mesh_index, indices_start, indices_end, the_number_of_triangles_accumulated_at_the_current_position}, ... }
    std::map<std::string, std::vector<int>> meshDict;

    // ----- Precalculate the number of triangulation and mesh index ----- //
    int totalTriangulation = 0, meshIndex = 0, index_pointer = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() == "Mesh") {
            // Get Visibility
            UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
            TfToken visibility;
            attr_visibility.Get(&visibility, timeCode);
            if (visibility == UsdGeomTokens->invisible) {
                continue;
            }

            std::vector<int> currentProcessMeshData;
            VtArray<int> vt_faceVertexCounts, vt_faceVertexIndices;

            UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
            UsdAttribute attr_faceVertexIndices = prim.GetAttribute(UsdGeomTokens->faceVertexIndices);

            attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);
            attr_faceVertexIndices.Get(&vt_faceVertexIndices, timeCode);

            currentProcessMeshData.emplace_back(meshIndex);
            currentProcessMeshData.emplace_back(index_pointer); // represent start pointer
            for (int vt_faceVertexCount : vt_faceVertexCounts) {
                totalTriangulation += vt_faceVertexCount - 2;
            }
            index_pointer += int(vt_faceVertexIndices.size());
            currentProcessMeshData.emplace_back(index_pointer); // represent end pointer
            currentProcessMeshData.emplace_back(totalTriangulation);

            meshDict[prim.GetPath().GetString()] = currentProcessMeshData;
            meshIndex++;
        }
    }

    // ----- Preallocate memory ----- //
    geometry_data[timeCode.GetValue()] = VertexData();
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
                    UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
                    TfToken visibility;
                    attr_visibility.Get(&visibility, timeCode);
                    if (visibility == UsdGeomTokens->invisible) {
                        return;
                    }
                }

                myTimer subtimer;
                subtimer.setStartPoint("Deal with Properties");

                VtArray<int> vt_faceVertexCounts;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
                attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

                std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
                // Range: [ start_pointer, end_pointer - 1 ]
                int start_pointer = mesh_data[1]; // include this value
                int end_pointer = mesh_data[2]; // except this value, end_pointer - 1;
                int number_of_triangulation = mesh_data[3];

                // Get UV Attribute Token
                bool has_uv;
                TfToken tf_uv;
                this->getUVToken(prim, tf_uv, has_uv);

                // Check Normal Attribute
                bool has_normal = prim.HasProperty(UsdGeomTokens->normals);

                // USD Attributes
                VtArray<int> vt_faceVertexIndices;
                VtArray<GfVec3f> vt_normals, vt_points;
                VtArray<GfVec2f> vt_uvs;
                VtArray<int> vt_uv_indices, vt_holeIndices;
                TfToken tf_normal_interpolation = TfToken("constant");
                TfToken tf_uv_interpolation = TfToken("constant");
                GfMatrix4d ModelTransform{};
                TfToken vt_orientation;

                VtVec3iArray vt_triFaceVertexIndices;
                VtIntArray vt_primitiveParam;
                VtVec3iArray vt_triEdgeIndices;
                //VtIntArray vt_triEdgeIndices;

                // Get Attribute Values
                UsdGeomMesh processGeom(prim);
                UsdAttribute attr_geoHole = processGeom.GetHoleIndicesAttr();
                UsdAttribute attr_faceVertexIndices = prim.GetAttribute(UsdGeomTokens->faceVertexIndices);
                UsdAttribute attr_normals = prim.GetAttribute(UsdGeomTokens->normals);
                UsdAttribute attr_points = prim.GetAttribute(UsdGeomTokens->points);
                UsdAttribute attr_uv = prim.GetAttribute(tf_uv);
                UsdAttribute attr_orientation = prim.GetAttribute(UsdGeomTokens->orientation);

                attr_faceVertexIndices.Get(&vt_faceVertexIndices, timeCode);
                attr_normals.Get(&vt_normals, timeCode);
                attr_points.Get(&vt_points, timeCode);
                attr_uv.Get(&vt_uvs, timeCode);
                attr_geoHole.Get(&vt_holeIndices, timeCode);
                attr_orientation.Get(&vt_orientation, timeCode);

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
                int faceVertexIndices = vt_faceVertexIndices.size();
                int reorder_index = 0;
                for (const auto index : vt_faceVertexIndices) {
                    vt_faceVertexIndices_reorder[reorder_index] = reorder_index;
                    reorder_index += 1;
                }

                tbb::parallel_for(0, faceVertexIndices, 1,
                                  [this, &vt_faceVertexIndices, &ModelTransform, &start_pointer,
                                          vt_points, vt_normals, vt_uv_indices, vt_uvs, &has_normal, &tf_normal_interpolation,
                                          &has_uv, &tf_uv_interpolation, &vertex_data](int faceVertexIndex_index) {

                // int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, ...]
                int faceVertexIndex = vt_faceVertexIndices[faceVertexIndex_index];

                // ----- Point ----- //
                GfVec3f vt_point = ModelTransform.Transform(vt_points[faceVertexIndex]);
//                qDebug() << "test point";
                vertex_data.vt_gl_position[faceVertexIndex_index + start_pointer] = vt_point;

                // e.g.   int[] faceVertexIndices     = [2, 0, 1, 3, 5, 4, 2, 3]
                //     -> int[] faceVertexIndices_new = [0, 1, 2, 3, 4, 5, 6, 7]
//                vt_faceVertexIndices_reorder[faceVertexIndex_index] = faceVertexIndex_index;

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

                subtimer.setStartPoint("Compute TriangleIndices");
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

                QString info_triangulation = QString("Compute TriangleIndices <") + QString(prim.GetPath().GetString().c_str()) + QString(">");
                subtimer.setEndPoint(info_triangulation.toStdString());

            });

    vertex_data.vt_gl_position.resize(actually_points);
    vertex_data.vt_gl_texCoord.resize(actually_points);
    vertex_data.vt_gl_normal.resize(actually_points);
    //qDebug() << "indices size: " << vertex_data.indices.size();

    // TODO assume each frame that has different indices for now
    // m_has_triangulated = true;

    m_timer.setEndPoint();

}

void usdParser::getDataBySpecifyFrame_TBB_Optimize_Triangulation(UsdTimeCode timeCode) {
    auto search = geometry_data.find(timeCode.GetValue());
    if(search != geometry_data.end()){
        //qDebug() << "this timeCode " << timeCode.GetValue() << " is already in the geometry_data";
        return;
    }
    if(timeCode < stage->GetStartTimeCode() || timeCode > stage->GetEndTimeCode()){
        //qDebug() << "this is not a valid timeCode " << timeCode.GetValue();
        return;
    }

    spdlog::info("\tGet data by specify frame: {}", timeCode.GetValue());

    myTimer m_timer;
    m_timer.setStartPoint("TraverseAll");
    m_timer.setStartPoint("TraverseFaceVertexCounts");

    // ----- Mesh dictionary ----- //
    // { "mesh1":{0, 0, 4, 2}, "mesh2":{1, 4, 28, 14}, ... }
    // { "mesh_name": {mesh_index, indices_start, indices_end, the_number_of_triangles_accumulated_at_the_current_position}, ... }
    std::map<std::string, std::vector<int>> meshDict;

    // ----- Precalculate the number of triangulation and mesh index ----- //
    int totalTriangulation = 0, meshIndex = 0, index_pointer = 0;
    for (UsdPrim prim: stage->TraverseAll()) {
        if (prim.GetTypeName() != "Mesh") { continue; }

        // Get Visibility
        UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
        TfToken visibility;
        attr_visibility.Get(&visibility, timeCode);

        if (visibility == UsdGeomTokens->invisible) { continue; }

        std::vector<int> currentProcessMeshData(5);
        VtArray<int> vt_faceVertexCounts;

        UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);

        attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

        currentProcessMeshData[0] = meshIndex;
        currentProcessMeshData[1] = index_pointer; // represent start pointer
        currentProcessMeshData[2] = totalTriangulation; // represent start pointer for triangle indices
        for (int vt_faceVertexCount : vt_faceVertexCounts) {
            totalTriangulation += vt_faceVertexCount - 2;
            index_pointer += vt_faceVertexCount;
        }
        currentProcessMeshData[3] = index_pointer; // represent end pointer
        currentProcessMeshData[4] = totalTriangulation;

        meshDict[prim.GetPath().GetString()] = currentProcessMeshData;
        meshIndex++;
    }
    m_timer.setEndPoint();

    // ----- Preallocate memory ----- //
    geometry_data[timeCode.GetValue()] = VertexData();
    VertexData &vertex_data = geometry_data[timeCode.GetValue()];
    vertex_data.indices.resize(totalTriangulation * 3); // 3 points per triangulation
    vertex_data.vt_gl_position.resize(index_pointer);
    vertex_data.vt_gl_texCoord.resize(index_pointer);
    vertex_data.vt_gl_normal.resize(index_pointer);
    vertex_data.vt_gl_display_color.resize(index_pointer);

    // ----- Actually the number of points, uv, normal ----- //
    int actually_points = 0, actually_uv = 0, actually_normal = 0;

    // ----- Parallel traverse all prim ----- //
    tbb::parallel_do(
            stage->TraverseAll(),
            [this, &timeCode, &actually_points, &meshDict, &vertex_data ](UsdPrim prim) {
                if (prim.GetTypeName() != "Mesh") {
                    return;
                } else {
                    UsdAttribute attr_visibility = prim.GetAttribute(UsdGeomTokens->visibility);
                    TfToken visibility;
                    attr_visibility.Get(&visibility, timeCode);
                    if (visibility == UsdGeomTokens->invisible) {
                        return;
                    }
                }

                myTimer subtimer;
                subtimer.setStartPoint("Deal with Properties");

                VtArray<int> vt_faceVertexCounts;
                UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
                attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);

                std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
                // Range: [ start_pointer, end_pointer - 1 ]
                int mesh_index = mesh_data[0]; //
                int start_pointer = mesh_data[1]; // faceVertexIndices start
                int start_tri_pointer = mesh_data[2]; // triangle amount start index
                int end_pointer = mesh_data[3]; // except this value, end_pointer - 1;
                int number_of_triangulation = mesh_data[4]; // triangle amount

                // Get UV Attribute Token
                bool has_uv;
                TfToken tf_uv;
                this->getUVToken(prim, tf_uv, has_uv);

                // Check Normal Attribute
                bool has_normal = prim.HasProperty(UsdGeomTokens->normals);

                // USD Attributes
                VtArray<int> vt_faceVertexIndices;
                VtArray<GfVec3f> vt_normals, vt_points, vt_displayColor;
                VtArray<GfVec2f> vt_uvs;
                VtArray<int> vt_uv_indices, vt_display_color_indices, vt_holeIndices;
                GfMatrix4d ModelTransform{};
                TfToken vt_orientation;

                // Get Attribute Values
                UsdGeomMesh processGeom(prim);
                UsdAttribute attr_geoHole = processGeom.GetHoleIndicesAttr();
                UsdAttribute attr_faceVertexIndices = prim.GetAttribute(UsdGeomTokens->faceVertexIndices);
                UsdAttribute attr_normals = prim.GetAttribute(UsdGeomTokens->normals);
                UsdAttribute attr_points = prim.GetAttribute(UsdGeomTokens->points);
                UsdAttribute attr_uv = prim.GetAttribute(tf_uv);
                UsdAttribute attr_displayColor = prim.GetAttribute(UsdGeomTokens->primvarsDisplayColor);
                UsdAttribute attr_orientation = prim.GetAttribute(UsdGeomTokens->orientation);

                attr_faceVertexIndices.Get(&vt_faceVertexIndices, timeCode);
                attr_normals.Get(&vt_normals, timeCode);
                attr_points.Get(&vt_points, timeCode);
                attr_uv.Get(&vt_uvs, timeCode);
                attr_displayColor.Get(&vt_displayColor, timeCode);
                attr_geoHole.Get(&vt_holeIndices, timeCode);
                attr_orientation.Get(&vt_orientation, timeCode);

//                spdlog::info("vt_displayColor\n{}", vt_displayColor);

                // Get interpolation
                // normal
                UsdGeomPrimvar primvar_normal(attr_normals);
                TfToken tf_normal_interpolation = primvar_normal.GetInterpolation();
                // uv
                UsdGeomPrimvar primvars_uv(attr_uv);
                TfToken tf_uv_interpolation = primvars_uv.GetInterpolation();
                primvars_uv.GetIndices(&vt_uv_indices, timeCode);
                // displayColor
                UsdGeomPrimvar primvar_displayColor(attr_displayColor);
                TfToken tf_displayColor_interpolation = primvar_displayColor.GetInterpolation();
                primvar_displayColor.GetIndices(&vt_display_color_indices, timeCode);

                // Define xformable for the final position calculation of each point
                UsdGeomXformable xformable(prim);
                ModelTransform = xformable.ComputeLocalToWorldTransform(timeCode);


                // Get Values
                int faceVertexCounts_size = vt_faceVertexCounts.size();
                int face_vertex_index_start = 0;

                // vt_faceVertexCounts -> int[] faceVertexCounts = [4, 4, 4, 4]
                // face_vertex_count_index: 0->1->2->3->...
                for (int face_vertex_count_index = 0; face_vertex_count_index < faceVertexCounts_size; ++face_vertex_count_index) {

                    // face_vertex_count: 4->4->4->4->...
                    int face_vertex_count = vt_faceVertexCounts[face_vertex_count_index];

                    // vt_faceVertexIndices -> int[] faceVertexIndices = [0, 1, 4, 3, 1, 2, 5, 4, 3, 4, 7, 6, 4, 5, 8, 7]
                    // face_vertex_index: 0->1->2->3->4->5->6->7->...
                    for (int face_vertex_index = face_vertex_index_start; face_vertex_index < face_vertex_index_start + face_vertex_count; ++face_vertex_index) {

                        // face_vertex: 0->1->4->3->...
                        int face_vertex = vt_faceVertexIndices[face_vertex_index];

                        // ----- Point ----- //
                        vertex_data.vt_gl_position[face_vertex_index + start_pointer] = ModelTransform.Transform(vt_points[face_vertex]);


                        // ----- Normal ----- //
                        if (vt_normals.empty()) {
                            // Set Normal
                             vertex_data.vt_gl_normal[face_vertex_index + start_pointer] = GfVec3f(0.0, 1.0, 0.0);
                        }
                        else {
                            int normal_index = 0;
                            // general
                            if (tf_normal_interpolation == "vertex") {
                                normal_index = face_vertex;
                            } else if (tf_normal_interpolation == "faceVarying") {
                                normal_index = face_vertex_index;
                            }

                            // Set Normal
                            vertex_data.vt_gl_normal[face_vertex_index + start_pointer] = vt_normals[normal_index];
                        }


                        // ----- UV ----- //
                        if (vt_uvs.empty()) {
                            // Set UV
//                             vertex_data.vt_gl_texCoord[face_vertex_index + start_pointer] = GfVec2f(0.0, 0.0);
                        } else {
                            int uv_index = 0;
                            // houdini
                            if (vt_uv_indices.empty()) {
                                if (tf_uv_interpolation == "vertex") {
                                    uv_index = face_vertex;
                                } else if (tf_uv_interpolation == "faceVarying") {
                                    uv_index = face_vertex_index;
                                }
                            }
                            // maya
                            else {
                                if (tf_uv_interpolation == "vertex") {
                                    uv_index = vt_uv_indices[face_vertex];
                                } else if (tf_uv_interpolation == "faceVarying") {
                                    uv_index = vt_uv_indices[face_vertex_index];
                                }
                            }
                            // Set UV
                            vertex_data.vt_gl_texCoord[face_vertex_index + start_pointer] = vt_uvs[uv_index];
                        }


                        // ----- DisplayColor ----- //
                        if (vt_displayColor.empty()) {
                            // Set Color
                            vertex_data.vt_gl_display_color[face_vertex_index + start_pointer] = GfVec3f(0.5, 0.5, 0.5);
                        } else {
                            int display_color_index = 0;    // <- if "constant"
                            // houdini
                            if (vt_display_color_indices.empty()) {
                                if (tf_displayColor_interpolation == "vertex") {
                                    display_color_index = face_vertex;
                                } else if (tf_displayColor_interpolation == "faceVarying") {
                                    display_color_index = face_vertex_index;
                                } else if (tf_displayColor_interpolation == "uniform") {
                                    display_color_index = face_vertex_count_index;
                                }
                            }
                            // maya
                            else {
                                // if (tf_uv_interpolation == "vertex"), do not deal with it now
                                if (tf_displayColor_interpolation == "uniform") {
                                    display_color_index = vt_display_color_indices[face_vertex_count_index];
                                }
                            }

                            // Set Color
                            vertex_data.vt_gl_display_color[face_vertex_index + start_pointer] = vt_displayColor[display_color_index];
                        }


                    } // for loop for vt_faceVertexIndices

                    // face_vertex_index_start: 0->4->8->12->...
                    face_vertex_index_start += face_vertex_count;

                } // for loop for vt_faceVertexCounts



                subtimer.setEndPoint();

//                spdlog::info("start_tri_pointer - {} - {}", start_tri_pointer, prim.GetPath().GetString());

                subtimer.setStartPoint("Compute TriangleIndices");
                // ----- Simple Triangulation ----- //
                computeTriangleIndices(vt_faceVertexCounts, vertex_data, start_tri_pointer * 3, start_pointer, UsdGeomTokens->rightHanded);

//                QString info_triangulation = QString("Compute TriangleIndices <") + QString(prim.GetPath().GetString().c_str()) + QString(">");
                subtimer.setEndPoint(prim.GetPath().GetString());

            }); // tbb::parallel_do  stage->TraverseAll(),

    m_timer.setEndPoint();


    // ---------- Debug Test Triangulation ----------
//    myTimer tri_timer;
//    int prim_index = 0;
//    for (UsdPrim prim: stage->TraverseAll()) {
//
//        if (prim.GetTypeName() != "Mesh") { continue; }
//        spdlog::info("{} - prim - {}", prim_index, prim.GetPath().GetString());
//        // Get Visibility
//        TfToken visibility;
//        prim.GetAttribute(UsdGeomTokens->visibility).Get(&visibility, timeCode);
//        if (visibility == UsdGeomTokens->invisible) { continue; }
//
//        std::vector<int> mesh_data = meshDict[prim.GetPath().GetString()];
//        int start_pointer = mesh_data[1]; // triangle amount start index
//        int start_tri_pointer = mesh_data[2]; // triangle amount start index
//
//        VtArray<int> vt_faceVertexCounts;
//        UsdAttribute attr_faceVertexCounts = prim.GetAttribute(UsdGeomTokens->faceVertexCounts);
//        attr_faceVertexCounts.Get(&vt_faceVertexCounts, timeCode);
//
//
//        // ----- Simple Triangulation ----- //
//        tri_timer.setStartPoint("Compute TriangleIndices");
//        spdlog::info("start_tri_pointer - {} - {}", start_tri_pointer * 3, prim.GetPath().GetString());
//        int tri_amount = 0;
//        for (int i = 0; i < vt_faceVertexCounts.size(); ++i) {
//            tri_amount += (vt_faceVertexCounts[i] - 2) * 3;
//        }
//        spdlog::info("tri_amount - {}", tri_amount);
//
//        computeTriangleIndices(vt_faceVertexCounts, vertex_data, start_tri_pointer * 3, start_pointer, UsdGeomTokens->rightHanded);
//        QString info_triangulation = QString("Compute TriangleIndices <") + QString(prim.GetPath().GetString().c_str()) + QString(">");
//        tri_timer.setEndPoint(info_triangulation.toStdString());
//    }

}

void usdParser::initGeometrySufficient() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];
    qDebug() << "allocate "
             << vertex_data.vt_gl_position.size()
             << vertex_data.vt_gl_texCoord.size()
             << vertex_data.vt_gl_normal.size()
             << vertex_data.vt_gl_display_color.size()
             << vertex_data.indices.size();

//    int allocateConstant1 = 175877952;
//    int allocateConstant2 = 175877952;
//    int allocateConstant3 = 175877952;
//    int allocateConstant4 = 175877952;
//    int allocateConstant5 = 263816928;
    int allocateConstant1 = vertex_data.vt_gl_position.size();
    int allocateConstant2 = vertex_data.vt_gl_texCoord.size();
    int allocateConstant3 = vertex_data.vt_gl_normal.size();
    int allocateConstant4 = vertex_data.vt_gl_display_color.size();
    int allocateConstant5 = vertex_data.indices.size();


    vbos[0].bind();
    vbos[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[0].allocate(nullptr, allocateConstant1 * sizeof(GfVec3f));

    vbos[1].bind();
    vbos[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[1].allocate(nullptr, allocateConstant2 * sizeof(GfVec2f));

    vbos[2].bind();
    vbos[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[2].allocate(nullptr, allocateConstant3 * sizeof(GfVec3f));

    vbos[3].bind();
    vbos[3].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[3].allocate(nullptr, allocateConstant4 * sizeof(GfVec3f));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(nullptr, allocateConstant5 * sizeof(GLuint));
}

void usdParser::initGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];

    qDebug() << "allocate "
             << vertex_data.vt_gl_position.size()
             << vertex_data.vt_gl_texCoord.size()
             << vertex_data.vt_gl_normal.size()
             << vertex_data.vt_gl_display_color.size()
             << vertex_data.indices.size();

    vbos[0].bind();
    vbos[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[0].allocate(vertex_data.vt_gl_position.data(), vertex_data.vt_gl_position.size() * sizeof(GfVec3f));

    vbos[1].bind();
    vbos[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[1].allocate(vertex_data.vt_gl_texCoord.data(), vertex_data.vt_gl_texCoord.size() * sizeof(GfVec2f));

    vbos[2].bind();
    vbos[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbos[2].allocate(vertex_data.vt_gl_normal.data(), vertex_data.vt_gl_normal.size() * sizeof(GfVec3f));

//    vbos[3].bind();
//    vbos[3].setUsagePattern(QOpenGLBuffer::StaticDraw);
//    vbos[3].allocate(vertex_data.vt_gl_display_color.data(), vertex_data.vt_gl_display_color.size() * sizeof(GfVec3f));

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

    vbos[vboIndex].bind();
    int displayColorLocation = program->attributeLocation("aDisplayColor");
    program->enableAttributeArray(displayColorLocation);
    program->setAttributeBuffer(displayColorLocation, GL_FLOAT, 0, 3, sizeof(GfVec3f));
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

void usdParser::computeTriangleIndices(VtArray<int> &face_vertex_counts, VertexData &vertex_data, int ele_start_index, int face_start_index, const TfToken& orientation) {

    // "Right": p1 = 1, p2 = 2
    // "Left":  p1 = 2, p2 = 1
    int p1, p2;
    if (orientation == UsdGeomTokens->rightHanded) {
        p1 = 1; p2 = 2;
    } else {
        p1 = 2; p2 = 1;
    }

    for (const auto & face_vertex_count : face_vertex_counts) {

        // tri_index: 1, 2, 3, ...
        for (int tri_index = 0; tri_index < face_vertex_count - 2; ++tri_index) {
            // [0, 1, 2, 0, 2, 3]
            // [0, 0 + 1, 0 + 2, 0, 0 + 2, 0 + 3]

            vertex_data.indices[ele_start_index] = face_start_index;                        // 0, 0, 0
            vertex_data.indices[ele_start_index + 1] = face_start_index + tri_index + p1;   // 1, 2, 3
            vertex_data.indices[ele_start_index + 2] = face_start_index + tri_index + p2 ;  // 2, 3, 4
            ele_start_index += 3;

        } // for face_vertex_count - 2

        // face_vertex_counts: [4, 4, 5, 4]
        // 0 -> 4 -> 8 -> 13
        face_start_index += face_vertex_count;

    } // for face_vertex_counts

}

void usdParser::updateVertex() {
    qDebug() << "updating - CTimeCode: " << currentTimeCode.GetValue();
//    currentTimeCode = UsdTimeCode(currentTimeCode.GetValue() + 1.0);

    if(currentTimeCode.GetValue() <= stage->GetEndTimeCode()){
        getDataBySpecifyFrame_TBB_Optimize_Triangulation(currentTimeCode);
    }else{
        currentTimeCode = animStartFrame;
    }

    if(lastDrewTimeCode == currentTimeCode){
        return;
    }

    myTimer m_timer;
    m_timer.setStartPoint(QString(QString("VboWrite ")+QString::number(currentTimeCode.GetValue())).toStdString());
    auto& vertex_data = geometry_data[currentTimeCode.GetValue()];

    if(0){
        vbos[0].bind();
        vbos[0].write(0, vertex_data.vt_gl_position.data(), vertex_data.vt_gl_position.size()* sizeof(GfVec3f));
        vbos[0].release();

        vbos[1].bind();
        vbos[1].write(0, vertex_data.vt_gl_texCoord.data(), vertex_data.vt_gl_texCoord.size()* sizeof(GfVec2f));
        vbos[1].release();

        vbos[2].bind();
        vbos[2].write(0, vertex_data.vt_gl_normal.data(), vertex_data.vt_gl_normal.size()* sizeof(GfVec3f));
        vbos[2].release();

        ebo.bind();
        ebo.write(0, vertex_data.indices.data(), vertex_data.indices.size()*sizeof(GLuint));
        ebo.release();
    }

//    vbos[0].bind();
//    GfVec3f* vbo0 = reinterpret_cast<GfVec3f*>(vbos[0].mapRange(0, vertex_data.vt_gl_position.size()* sizeof(GfVec3f), QOpenGLBuffer::RangeWrite));
//    std::copy(vertex_data.vt_gl_position.begin(), vertex_data.vt_gl_position.end(), vbo0);
//    vbos[0].release();

    if(0){
        vbos[0].bind();
        GfVec3f* vbo0 = reinterpret_cast<GfVec3f*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_position.size()* sizeof(GfVec3f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        std::copy(vertex_data.vt_gl_position.begin(), vertex_data.vt_gl_position.end(), vbo0);
        vbos[0].unmap();
        vbos[0].release();

        vbos[1].bind();
        GfVec2f* vbo1 = reinterpret_cast<GfVec2f*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_texCoord.size()* sizeof(GfVec2f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        std::copy(vertex_data.vt_gl_texCoord.begin(), vertex_data.vt_gl_texCoord.end(), vbo1);
        vbos[1].unmap();
        vbos[1].release();

        vbos[2].bind();
        GfVec3f* vbo2 = reinterpret_cast<GfVec3f*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_normal.size()* sizeof(GfVec3f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        std::copy(vertex_data.vt_gl_normal.begin(), vertex_data.vt_gl_normal.end(), vbo2);
        vbos[2].unmap();
        vbos[2].release();

        ebo.bind();
        void* ebo0 = ebo.mapRange(0, vertex_data.indices.size()* sizeof(GLuint), QOpenGLBuffer::RangeWrite);
        memcpy(ebo0, vertex_data.indices.data(), vertex_data.indices.size()* sizeof(GLuint));
        ebo.unmap();
        ebo.release();
    }

    if(1){
        vbos[0].bind();
        void* vbo0 = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_position.size()* sizeof(GfVec3f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(vbo0, vertex_data.vt_gl_position.data(), vertex_data.vt_gl_position.size()* sizeof(GfVec3f));
        vbos[0].unmap();
        vbos[0].release();

        vbos[1].bind();
        void* vbo1 = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_texCoord.size()* sizeof(GfVec2f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(vbo1, vertex_data.vt_gl_texCoord.data(), vertex_data.vt_gl_texCoord.size()* sizeof(GfVec2f));
        vbos[1].unmap();
        vbos[1].release();

        vbos[2].bind();
        void* vbo2 = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_normal.size()* sizeof(GfVec3f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(vbo2, vertex_data.vt_gl_normal.data(), vertex_data.vt_gl_normal.size()* sizeof(GfVec3f));
        vbos[2].unmap();
        vbos[2].release();

        vbos[3].bind();
        void* vbo3 = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_data.vt_gl_display_color.size()* sizeof(GfVec3f),
                                                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(vbo3, vertex_data.vt_gl_display_color.data(), vertex_data.vt_gl_display_color.size()* sizeof(GfVec3f));
        vbos[3].unmap();
        vbos[3].release();

        ebo.bind();
        void* ebo0 = ebo.mapRange(0, vertex_data.indices.size()* sizeof(GLuint), QOpenGLBuffer::RangeWrite);
        memcpy(ebo0, vertex_data.indices.data(), vertex_data.indices.size()* sizeof(GLuint));
        ebo.unmap();
        ebo.release();
    }

    m_timer.setEndPoint();
    lastDrewTimeCode = currentTimeCode;
}

void usdParser::getDataByAll() {
    UsdTimeCode startTimeCode = stage->GetStartTimeCode();
    UsdTimeCode endTimeCode = stage->GetEndTimeCode();
    std::vector<UsdTimeCode> timeCodes;
    while(startTimeCode <= endTimeCode){

        timeCodes.push_back(startTimeCode);
        startTimeCode = UsdTimeCode(startTimeCode.GetValue() + 1);
    }

    qDebug() << "allTimeCodes: " << timeCodes.size();

    tbb::parallel_for_each(
            timeCodes,
            [this](UsdTimeCode step)
    {
        getDataBySpecifyFrame_TBB_Optimize_Triangulation(UsdTimeCode(step));
    });
//    getDataBySpecifyFrame_optimized_non_TBB(1);
}
