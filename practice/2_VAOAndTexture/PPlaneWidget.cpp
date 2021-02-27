#include "PPlaneWidget.h"

QVector<VertexData> PPlaneWidget::getVerticesData() {
    return {
        {QVector3D(0.5f, 0.5f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector2D(1.0f, 1.0f)},
        {QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector2D(0.0f, 0.0f)},
        {QVector3D(0.5f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector2D(1.0f, 0.0f)},
        {QVector3D(-0.5f, 0.5f, 0.0f), QVector3D(1.0f, 1.0f, 0.0f), QVector2D(0.0f, 1.0f)}
    };
}

QVector<GLushort> PPlaneWidget::getIndices() {
    return {
        0, 1, 2,
        1, 0, 3
    };
}

void PPlaneWidget::initShaders() {
    program.addShaderFromSourceFile(QOpenGLShader::Vertex, "Plane.vs.glsl");
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, "Plane.fs.glsl");
    program.link();
}

void PPlaneWidget::initPlaneGeometry() {
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    ebo.bind();
    vbo.bind();
    program.bind();

    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.allocate(getVerticesData().constData(), 4 * sizeof(VertexData));

    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.allocate(getIndices().constData(), 6 * sizeof(GLushort));

    quintptr offset = 0;
    int vertex_lct = program.attributeLocation("a_position");
    int color_lct = program.attributeLocation("a_color");
    int coord_lct = program.attributeLocation("a_coord");
    qDebug() << "location: " << vertex_lct << " " << color_lct << " " << coord_lct;

    program.enableAttributeArray(vertex_lct);
    program.setAttributeBuffer(vertex_lct, GL_FLOAT, offset, 3, sizeof(VertexData));
    offset += sizeof(QVector3D);
    program.enableAttributeArray(color_lct);
    program.setAttributeBuffer(color_lct, GL_FLOAT, offset, 3, sizeof(VertexData));
    offset += sizeof(QVector3D);
    program.enableAttributeArray(coord_lct);
    program.setAttributeBuffer(coord_lct, GL_FLOAT, offset, 2, sizeof(VertexData));
}

void PPlaneWidget::initTextures() {
    QImage img1 = QImage(QString("awesomeface.png"));
    QImage img2 = QImage(QString("wall.jpg"));
    img1 = img1.mirrored();
    img2 = img2.mirrored();
    texture = new QOpenGLTexture(img1);
    texture1 = new QOpenGLTexture(img2);
}

void PPlaneWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.15, 0.15, 0.15, 1);

    program.bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    QMatrix4x4 model, view, mvp, projection;
    model.translate(0.0, 0.0, -2.0);
    model.rotate(-90.0f, 1, 0, 0);
    view.translate(0.0, -0.2 ,0.0);
    const qreal zNear = 0.0, zFar = 10.0, fov = 60.0;
    projection.perspective(fov, 1.0f, zNear, zFar);
    mvp = projection*view*model;
    qDebug() << "projection matrix: " << projection;
    qDebug() << "MVP matrix: " << mvp;

    program.setUniformValue("mvp_matrix", mvp);
    program.setUniformValue("map", 0);
    texture->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

PPlaneWidget::PPlaneWidget(QWidget *widget) :
    PMainWidget(widget),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer)
{
    qDebug() << "Constructor called";
}

void PPlaneWidget::initializeGL() {
    qDebug() << "initializeGL called";
    initializeOpenGLFunctions();
    vbo.create();
    ebo.create();
    initShaders();
    initTextures();
    initPlaneGeometry();
}

PPlaneWidget::~PPlaneWidget() {
    makeCurrent();
    delete texture;
    delete texture1;
    doneCurrent();
}

void PPlaneWidget::resizeGL(int w, int h) {
    qDebug() << "resize GL called";
//    const qreal zNear = 0.0, zFar = 7.0, fov = 45.0;
//    projection.setToIdentity();
//    projection.perspective(fov, qreal(w)/qreal(h), zNear, zFar);
}

