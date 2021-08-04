#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QMatrix4x4>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QtMath>
#include <QKeyEvent>
#include <QString>
#include <QDebug>
#include <iostream>

#include <QApplication>


#pragma push_macro("slots")
#undef slots

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/base/vt/types.h"

#pragma pop_macro("slots")

#include "spdlog/spdlog.h"
#include "boost/log/trivial.hpp"


struct VertexData
{
    QVector3D position;
};


class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void initShaders();
    void initTexture();
    void initGLSetting();
    void initGeometry();
    void setupAttributePointer();

    QVector<VertexData> getVerticesData();
    QVector<GLuint> getIndices();

    void testReadUSDFile();
    void testTriangulation();
    void testPointAnimation();

private:
    QOpenGLShaderProgram *program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;

    QMatrix4x4 projection;
    QQuaternion rotation;
    QVector<VertexData> vertices;
    QVector<GLuint> indices;

public slots:
    void cleanup();
};

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          program(nullptr),
          ebo(QOpenGLBuffer::IndexBuffer) {

}

GLWidget::~GLWidget() {
    cleanup();
}

QSize GLWidget::minimumSizeHint() const {
    return {50, 50};
}

QSize GLWidget::sizeHint() const {
    return {360, 360};
}

void GLWidget::initializeGL() {
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    // --------------------------------------- //
    initGLSetting();

//    initShaders();
//    initTexture();

//    testReadUSDFile();
//    testTriangulation();
    testPointAnimation();

//    initGeometry();
//    setupAttributePointer();

}

void GLWidget::paintGL() {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program->bind();
    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0.0, -3.0);

    program->setUniformValue("mvp_matrix", projection * matrix);

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    glDrawElements(GL_TRIANGLES, getIndices().count(), GL_UNSIGNED_INT, (void*)nullptr);
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);

    // Set near plane to 0.0001, far plane to 1000.0, field of view 45 degrees
    // 摄像机望向z负方向，能看到范围是-0.0001 ~ -1000.0
    const qreal zNear = 0.0001, zFar = 1000.0, fov = 45.0;

    // Reset projection
//    projection.setToIdentity();

    // Set perspective projection
//    projection.perspective(fov, aspect, zNear, zFar);
}

void GLWidget::cleanup() {
    spdlog::info("GLWidget cleanup");
    if (program == nullptr)
        return;

    makeCurrent();
    delete program;
    program = nullptr;
    vbo.destroy();
    ebo.destroy();
    doneCurrent();
}

void GLWidget::initShaders() {
    program = new QOpenGLShaderProgram(this);

    // ----- add shader from source file/code ----- //
    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/22_UDIM/udim.vs.glsl"))
        close();

    if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/22_UDIM/udim.fs.glsl"))
        close();

    if (!program->link())
        close();

    if (!program->bind())
        close();
}

void GLWidget::initGeometry() {

    vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    vbo.create();
    ebo.create();

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.bind();
    vbo.allocate(getVerticesData().constData(), getVerticesData().count() * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.bind();
    ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
}

void GLWidget::initGLSetting() {
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    // Enable back face culling
    glEnable(GL_CULL_FACE);
}

void GLWidget::initTexture() {

}

void GLWidget::setupAttributePointer() {
    // --------------------------------------------------------------------------------------- //
    // Offset for position
    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
//    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
//    int colorLocation = program->attributeLocation("aColor");
//    program->enableAttributeArray(colorLocation);
//    program->setAttributeBuffer(colorLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    program->release();
}

void GLWidget::testReadUSDFile() {
    QString path = "src/22_UDIM/resource/test_cube_udim/main.usd";
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(path.toStdString());
    for (pxr::UsdPrim prim: stage->TraverseAll()) {
//        if (!prim.IsA<pxr::UsdGeomMesh>()) { continue; }
        spdlog::debug("prim --- {}", prim.GetPath().GetNameToken().data());
    }
    spdlog::debug("stage traversed.");
}

void GLWidget::testTriangulation() {

    QString path = "src/22_UDIM/resource/test_triangulation/main.usd";
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(path.toStdString());

    pxr::UsdTimeCode time(0);

    pxr::UsdPrim prim = stage->GetPrimAtPath(pxr::SdfPath("/pPlane1"));

    pxr::UsdAttribute attr_faceVertexCounts = prim.GetAttribute(pxr::TfToken("faceVertexCounts"));
    pxr::VtArray<int> faceVertexCounts;    // int[] faceVertexCounts = [4]
    attr_faceVertexCounts.Get(&faceVertexCounts, time);
    BOOST_LOG_TRIVIAL(debug) << ">>> faceVertexCounts \n" << faceVertexCounts;

    pxr::UsdAttribute attr_faceVertexIndices = prim.GetAttribute(pxr::TfToken("faceVertexIndices"));
    pxr::VtArray<int> faceVertexIndices;    // int[] faceVertexIndices = [0, 1, 3, 2]
    attr_faceVertexIndices.Get(&faceVertexIndices, time);
    BOOST_LOG_TRIVIAL(debug) << ">>> faceVertexIndices \n" << faceVertexIndices;

    pxr::VtIntArray holeIndices(0);


//    ---------- Triangulation ----------
    pxr::HdMeshTopology topology(
        pxr::UsdGeomTokens->none, pxr::UsdGeomTokens->rightHanded,
        faceVertexCounts, faceVertexIndices, holeIndices);

    pxr::VtVec3iArray trianglesFaceVertexIndices;
    pxr::VtIntArray primitiveParam;
    pxr::VtVec3iArray trianglesEdgeIndices;

    pxr::HdMeshUtil mesh_util(&topology, prim.GetPath());
    mesh_util.ComputeTriangleIndices(&trianglesFaceVertexIndices, &primitiveParam, &trianglesEdgeIndices);

    BOOST_LOG_TRIVIAL(debug) << ">>> trianglesFaceVertexIndices \n" << trianglesFaceVertexIndices;    // [(0, 1, 3), (0, 3, 2)]
    BOOST_LOG_TRIVIAL(debug) << ">>> primitiveParam \n" << primitiveParam;    // [1, 2]
    BOOST_LOG_TRIVIAL(debug) << ">>> trianglesEdgeIndices \n" << trianglesEdgeIndices;    // [(0, 1, -1), (-1, 2, 3)]
}


void GLWidget::testPointAnimation() {
    QString path = "src/22_UDIM/resource/test_point_anim/main.usd";
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(path.toStdString());

    std::string result;
    stage->ExportToString(&result);
    BOOST_LOG_TRIVIAL(debug) << result;

}

QVector<VertexData> GLWidget::getVerticesData() {
    return vertices;
}

QVector<GLuint> GLWidget::getIndices() {
    return indices;
}



int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::info("Welcome to spdlog!");

    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(16);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication::setApplicationName("Point Cache Animation");

    GLWidget w;
    w.show();

    return QApplication::exec();
}

#include "main.moc"
