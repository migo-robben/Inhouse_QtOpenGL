#include <QSurfaceFormat>
#include <QApplication>
#include <QString>
#include <QDebug>

#include "GLWidget.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/imaging/hd/meshUtil.h"


bool fanTriangulate(GfVec3i &dst, pxr::VtArray<int> const &src, int offset, int index, int size, bool flip) {
    if (offset + index + 2 >= size) {
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

bool testComputeTriangleIndices(VtArray<int> &faceVertexCounts, VtArray<int> &faceVertexIndices, const TfToken& orientation, VtVec3iArray &tri_indices) {
    int numFaces = faceVertexCounts.size();
    int numVertIndices = faceVertexIndices.size();
    int numTris = 0;

    bool invalidTopology = false;

    for (int i = 0; i < numFaces; ++i) {
        int nv = faceVertexCounts[i] - 2;
        if (nv < 1) {
            invalidTopology = true;
            return invalidTopology;
        } else {
            numTris += nv;
        }
    }

    tri_indices.resize(numTris);
    bool flip = (orientation != UsdGeomTokens->rightHanded);

    for (int i=0,tv=0,v=0; i<numFaces; ++i) {
        int nv = faceVertexCounts[i];
        if (nv < 3) {
        } else {
            for (int j=0; j < nv-2; ++j) {
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

void testTriangulation() {
    QString path = "F:/temp/plane_box.usda";
    UsdStageRefPtr stage = UsdStage::Open(path.toStdString());

    UsdTimeCode time(0);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/box/box_mesh"));

    UsdAttribute attr_faceVertexCounts = prim.GetAttribute(TfToken("faceVertexCounts"));
    VtArray<int> faceVertexCounts;    // int[] faceVertexCounts = [4]
    attr_faceVertexCounts.Get(&faceVertexCounts, time);
    spdlog::info(">>> faceVertexCounts \n{}", faceVertexCounts);

    UsdAttribute attr_faceVertexIndices = prim.GetAttribute(TfToken("faceVertexIndices"));
    VtArray<int> faceVertexIndices;    // int[] faceVertexIndices = [0, 1, 3, 2]
    attr_faceVertexIndices.Get(&faceVertexIndices, time);
    spdlog::info(">>> faceVertexIndices \n{}", faceVertexIndices);

    VtIntArray holeIndices(0);

    // ---------- Triangulation ---------- //
    HdMeshTopology topology(
            UsdGeomTokens->none, UsdGeomTokens->rightHanded,
            faceVertexCounts, faceVertexIndices, holeIndices);

    VtVec3iArray trianglesFaceVertexIndices;
    VtIntArray primitiveParam;
    VtVec3iArray trianglesEdgeIndices;

    HdMeshUtil mesh_util(&topology, prim.GetPath());
    mesh_util.ComputeTriangleIndices(&trianglesFaceVertexIndices, &primitiveParam, &trianglesEdgeIndices);

    spdlog::info(">>> trianglesFaceVertexIndices \n{}", trianglesFaceVertexIndices);    // [(0, 1, 3), (0, 3, 2)]
    spdlog::info(">>> primitiveParam \n{}", primitiveParam);    // [1, 2]
    spdlog::info(">>> trianglesEdgeIndices \n{}", trianglesEdgeIndices);    // [(0, 1, -1), (-1, 2, 3)]

    // ----- my triangulation ----- //
    VtVec3iArray tri_indices;
    testComputeTriangleIndices(faceVertexCounts, faceVertexIndices, UsdGeomTokens->rightHanded, tri_indices);
    spdlog::info(">>> trianglesFaceVertexIndices \n{}", tri_indices);
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setApplicationName("Read USD");

    GLWidget w;
    w.show();

//    testTriangulation();

    return QApplication::exec();
}
