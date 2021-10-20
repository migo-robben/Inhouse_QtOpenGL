#ifndef QTREFERENCE_USDPARSER_H
#define QTREFERENCE_USDPARSER_H

#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")

#define NOMINMAX
#undef snprintf

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/imaging/hd/meshUtil.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#include "myTimer.h"

#include <QString>
#include <QVector2D>
#include <QVector3D>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLVertexArrayObject>

#include <QDebug>

#include <vector>
#include <mutex>

#include <tbb/tbb.h>

using namespace pxr;

class usdParser : protected QOpenGLFunctions_4_5_Core {
public:
    explicit usdParser(QString &path);

    void getUVToken(UsdPrim &prim, TfToken &tf_uv, bool &uvs);
    void getDataBySpecifyFrame_TBB_with_simpleTriangulation(UsdTimeCode timeCode);

    void setupAttributePointer(QOpenGLShaderProgram *program);
    void setupAttributePointerWireframe(QOpenGLShaderProgram *program);

    void initGeometryBuffer();
    void initGeometryBufferWireframe();

    void initGeometryMapRange();
    void initGeometryWireframeMapRange();

    void initGeometryDefault();

    void drawGeometry(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection);
    void drawGeometryWireframe(QOpenGLShaderProgram *program, QMatrix4x4 model, QMatrix4x4 view, QMatrix4x4 projection);

    bool fanTriangulate(GfVec3i &dst, VtArray<int> const &src, int offset, int index, int size, bool flip);
    bool simpleComputeTriangleIndices(VtArray<int> &faceVertexCounts, VtArray<int> &faceVertexIndices,
                                      const TfToken& orientation, VtVec3iArray &tri_indices);

    void parseMeshWireframe();

    void debugTest();
    void debugTBBTest();
    void debugWireFrame();
    void debugCustomFunc() {};
    void debugInfo();
private:
    UsdStageRefPtr stage;
    QString usdFilePath;

public:
    double fps;
    double animStartFrame;
    double animEndFrame;
    UsdTimeCode currenTimeCode;
    int attributeCount = 3;

protected:
    QOpenGLVertexArrayObject vaoGeometry;
    QVector<QOpenGLBuffer> vbos;
    QOpenGLBuffer ebo;

    std::vector<QVector3D> position;
    std::vector<QVector2D> texCoord;
    std::vector<QVector3D> normal;

    VtVec3fArray vt_gl_position;
    VtVec2fArray vt_gl_texCoord;
    VtVec3fArray vt_gl_normal;

    std::vector<GLuint> indices;

    // ----- Geometry wireframe ----- //
    QOpenGLVertexArrayObject vaoWireframe;
    QOpenGLBuffer vboWireframe;
    std::vector<GLuint> indices_wireframe;
    VtVec3fArray vt_gl_position_wireframe;

    bool m_has_triangulated = false;
    int m_indicesCount = 0;
    int m_indexIncrease = 0;

    std::mutex mtx;

    QSet<QPair<int, int>> wireframeSet;
};

#endif //QTREFERENCE_USDPARSER_H
