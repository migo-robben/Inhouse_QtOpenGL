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
#include <QString>
#include <QKeyEvent>

#include <QApplication>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/ostreamwrapper.h>

#include <fstream>

using namespace rapidjson;

struct VertexData
{
    QVector3D position;
    QVector3D color;
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void keyPressEvent(QKeyEvent *event) override;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void initShaders();
    void initGeometry();

    QVector<VertexData> getVerticesData();
    void _getVerticesData(QString frameStep);
    QVector<GLuint> getIndices();

    void updateVerticesData();

private:
    QOpenGLShaderProgram *program;
    QOpenGLVertexArrayObject vao;

    QMatrix4x4 projection;
    QQuaternion rotation;

    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;

    QVector<VertexData> vertices;
    QVector<GLuint> indices;

    QElapsedTimer elapsedTimer;
    QTimer *timer;
    float deltaTime = 0.0;
    float timeStampBeforeUpdataVBO = 0.0;

    bool triangleDraw = false;

    bool switcher = false; //  Press A key to change switcher status
    bool finishedTotalFrame = false;

    enum MODE {
        DEFAULT = 1,
        MODEONE,
        MODESECOND
    };
    MODE mode = MODESECOND;

    QString cacheFilePath;
    Document document;
    int duration;
    int startFrame;
    int endFrame;
    int fps;
    int pointSize;
    int currentFrame = 1;

public slots:
    void cleanup();
};

GLWidget::GLWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          program(nullptr),
          ebo(QOpenGLBuffer::IndexBuffer) {

    if (!triangleDraw) {
        cacheFilePath = QString("src/21_PointCacheAnimation/cache/HelixData.json");
        std::ifstream ifs(cacheFilePath.toStdString());
        IStreamWrapper isw(ifs);
        document.ParseStream(isw);

        duration = document["duration"].GetInt();
        startFrame = document["startFrame"].GetInt();
        endFrame = document["endFrame"].GetInt();
        fps = document["fps"].GetInt();
        pointSize = document["pointSize"].GetInt();

        vertices.clear();
        vertices.empty();
        indices.clear();
        indices.empty();

        qDebug() << "Duration:" << duration << ", start frame:" << startFrame << ", end frame:" << endFrame << ", fps:" << fps << ", point count:" << pointSize;
    }
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

    program = new QOpenGLShaderProgram(this);
    // ----- add shader from source file/code ----- //
    initShaders();
    // --------------------------------------- //

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    // Enable back face culling
    glEnable(GL_CULL_FACE);

    vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    initGeometry();

    // --------------------------------------------------------------------------------------- //
    // Offset for position
    quintptr offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("aPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int colorLocation = program->attributeLocation("aColor");
    program->enableAttributeArray(colorLocation);
    program->setAttributeBuffer(colorLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    program->release();

    // update frame setting
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GLWidget::updateVerticesData);
    timer->start(30);
    elapsedTimer.start();
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

    if (triangleDraw)
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)nullptr);
    else
        glDrawElements(GL_TRIANGLES, getIndices().count(), GL_UNSIGNED_INT, (void*)nullptr);
}

void GLWidget::resizeGL(int width, int height) {
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);

    // Set near plane to 0.0001, far plane to 1000.0, field of view 45 degrees
    // 摄像机望向z负方向，能看到范围是-0.0001 ~ -1000.0
    const qreal zNear = 0.0001, zFar = 1000.0, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
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
    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/21_PointCacheAnimation/helix.vs.glsl"))
        close();

    if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/21_PointCacheAnimation/helix.fs.glsl"))
        close();

    if (!program->link())
        close();

    if (!program->bind())
        close();
}

void GLWidget::initGeometry() {
    initializeOpenGLFunctions();

    vbo.create();
    ebo.create();

    if (triangleDraw) {
        vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        vbo.bind();
        vbo.allocate(&getVerticesData()[0], 3 * sizeof(VertexData));

        ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        ebo.bind();
        ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));

    }
    else {
        vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        vbo.bind();
        vbo.allocate(&getVerticesData()[0], getIndices().count() * sizeof(VertexData));

        ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        ebo.bind();
        ebo.allocate(getIndices().constData(), getIndices().count() * sizeof(GLuint));
    }
}

void GLWidget::_getVerticesData(QString frameStep) {
    auto data = document["data"].GetObj();

    const Value& pointsArray = data[frameStep.toStdString().c_str()].GetObj()["points"];
    const Value& normalArray = data[frameStep.toStdString().c_str()].GetObj()["normal"];

    for (SizeType i = 0; i < pointsArray.Size(); i++) {
        VertexData tdata;
        auto index = QString::number(i).toStdString();
        tdata.position = QVector3D(
                pointsArray[index.c_str()].GetArray()[0].GetFloat(),
                pointsArray[index.c_str()].GetArray()[1].GetFloat(),
                pointsArray[index.c_str()].GetArray()[2].GetFloat());
        tdata.color = QVector3D(
                normalArray[index.c_str()].GetArray()[0].GetFloat(),
                normalArray[index.c_str()].GetArray()[1].GetFloat(),
                normalArray[index.c_str()].GetArray()[2].GetFloat());

        vertices.push_back(tdata);
    }
}

QVector<VertexData> GLWidget::getVerticesData() {

    if (triangleDraw) {
        vertices = {
                // Triangle 1
                {QVector3D(0.0f, 0.5f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)},
                {QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
                {QVector3D(0.5f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)},
                // Triangle 2
                {QVector3D(0.0f, 0.1f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)},
                {QVector3D(-0.75f, -0.5f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f)},
                {QVector3D(0.75f, -0.5f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)}
        };
    }
    else {
        if (vertices.isEmpty()) {
            if (mode == DEFAULT || mode == MODESECOND) {
                QString frame_str("frame_");
                frame_str += QString::number(currentFrame);

                _getVerticesData(frame_str);
            }
            else if (mode == MODEONE) {
                for (int i = startFrame; i <= endFrame; i++) {
                    QString frame_str("frame_");
                    frame_str += QString::number(i);
                    qDebug() << "-----> processing frame:" << frame_str;

                    _getVerticesData(frame_str);
                }
            }
        }
    }

    return vertices;
}

QVector<GLuint> GLWidget::getIndices() {
    if (triangleDraw) {
        indices = {
                0, 1, 2,
        };
    }
    else {
        if (indices.isEmpty()) {
            QString frame_str("frame_");
            frame_str += QString::number(currentFrame);

            auto data = document["data"].GetObj();
            auto indicesArray = data["indices"].GetArray();
            for (int i = 0; i < pointSize; i++) {
                indices.push_back(indicesArray[i].GetInt());
            }
        }
    }

    return indices;
}

void GLWidget::updateVerticesData() {

    if (triangleDraw) {
        if (switcher) {
            vertices[0].position += QVector3D(0.0, 0.01, 0.0);

            vbo.bind();
            vbo.write(0, &vertices[0], 3 * sizeof(VertexData));
            vbo.release();
        }
        else {
            vertices[0].position = QVector3D(0.0, 0.5, 0.0);

            vbo.bind();
            vbo.write(0, &vertices[3], 3 * sizeof(VertexData));
            vbo.release();
        }
    }
    else {
        if (mode == DEFAULT) {
            timeStampBeforeUpdataVBO = elapsedTimer.elapsed();

            // ----- update vertices array ----- //
            currentFrame = currentFrame++ % duration;
            qDebug() << "Current Frame:" << currentFrame;

            if (!vertices.isEmpty()) {
                vertices.empty();
                vertices.resize(0);
            }

            QString frame_str("frame_");
            frame_str += QString::number(currentFrame);

            auto data = document["data"].GetObj();

            auto pointsArray = data[frame_str.toStdString().c_str()].GetObj()["points"].GetObj();
            auto normalArray = data[frame_str.toStdString().c_str()].GetObj()["normal"].GetObj();

            for (int i = 0; i < pointSize; i++) {
                VertexData tdata;
                auto index = QString::number(i).toStdString();
                tdata.position = QVector3D(
                        pointsArray[index.c_str()].GetArray()[0].GetFloat(),
                        pointsArray[index.c_str()].GetArray()[1].GetFloat(),
                        pointsArray[index.c_str()].GetArray()[2].GetFloat());
                tdata.color = QVector3D(
                        normalArray[index.c_str()].GetArray()[0].GetFloat(),
                        normalArray[index.c_str()].GetArray()[1].GetFloat(),
                        normalArray[index.c_str()].GetArray()[2].GetFloat());

                vertices.push_back(tdata);
            }

            deltaTime = elapsedTimer.elapsed() - timeStampBeforeUpdataVBO;
            qDebug() << "   Update vertices array use:" << deltaTime / 1000.0 << " second.";

            // ----- update vbo ----- //
            timeStampBeforeUpdataVBO = elapsedTimer.elapsed();
            vbo.bind();
            vbo.write(0, vertices.constData(), getIndices().count() * sizeof(VertexData));
            vbo.release();

            deltaTime = elapsedTimer.elapsed() - timeStampBeforeUpdataVBO;
            qDebug() << "   Update vbo use:" << deltaTime / 1000.0 << " second.";
        }
        else if (mode == MODEONE) {
            timeStampBeforeUpdataVBO = elapsedTimer.elapsed();

            currentFrame = currentFrame++ % duration;
            qDebug() << "Current Frame:" << currentFrame;

            vbo.bind();
            vbo.write(0, &vertices[(currentFrame - 1) * getIndices().count()], getIndices().count() * sizeof(VertexData));
            vbo.release();

            deltaTime = elapsedTimer.elapsed() - timeStampBeforeUpdataVBO;
            qDebug() << "   Update vbo use:" << deltaTime / 1000.0 << " second.";
        }
        else if (mode == MODESECOND) {
            timeStampBeforeUpdataVBO = elapsedTimer.elapsed();

            currentFrame = currentFrame++ % duration;
            qDebug() << "Current Frame:" << currentFrame;

            if (!finishedTotalFrame) {

                QString frame_str("frame_");
                frame_str += QString::number(currentFrame);

                _getVerticesData(frame_str);

                if (currentFrame == endFrame) {
                    finishedTotalFrame = !finishedTotalFrame;
                }

                deltaTime = elapsedTimer.elapsed() - timeStampBeforeUpdataVBO;
                qDebug() << "   Update vertices array use:" << deltaTime / 1000.0 << " second.";
            }

            // ----- update vbo ----- //
            timeStampBeforeUpdataVBO = elapsedTimer.elapsed();
            vbo.bind();
            vbo.write(0, &vertices[(currentFrame - 1) * getIndices().count()], getIndices().count() * sizeof(VertexData));
            vbo.release();

            deltaTime = elapsedTimer.elapsed() - timeStampBeforeUpdataVBO;
            qDebug() << "   Update vbo use:" << deltaTime / 1000.0 << " second.";
        }
    }

    update();
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_A) {
        switcher = !switcher;
    }

    QWidget::keyPressEvent(event);
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

    // ----- Test rapidjson ----- //

    return QApplication::exec();
}

#include "main.moc"