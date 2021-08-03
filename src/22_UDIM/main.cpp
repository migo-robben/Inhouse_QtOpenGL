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

#include <QApplication>


#pragma push_macro("slots")
#undef slots

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"

#pragma pop_macro("slots")

#include <boost/log/trivial.hpp>


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

    testReadUSDFile();
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
        BOOST_LOG_TRIVIAL(debug) << "prim --- " << prim.GetPath();
    }
}

QVector<VertexData> GLWidget::getVerticesData() {
    return vertices;
}

QVector<GLuint> GLWidget::getIndices() {
    return indices;
}



int main(int argc, char *argv[]) {
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
