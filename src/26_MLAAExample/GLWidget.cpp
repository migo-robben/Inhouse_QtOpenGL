#include "GLWidget.h"

#include <QComboBox>
#include <QKeyEvent>

#define SHADER(x) programs[x]

MainWidget::MainWidget(QWidget *parent)
        : QOpenGLWidget(parent),
          gridGeometry(nullptr),
          srcTexture(nullptr),
          areaTexture(nullptr),
          edgeRT(nullptr),
          blendWeightRT(nullptr),
          neighborhoodBlendingRT(nullptr),
          blendWeightBufferTexture(nullptr),
          blendWeightAlphaBufferTexture(nullptr),
          simpleDemo(false)
{
    // Create two shader program
    // the first one use for offscreen rendering
    // the second for default framebuffer rendering
    for (int i = 0; i < 4; i++) {
        programs.push_back(new QOpenGLShaderProgram(this));
    }

    indexMapArea[0] = 9;
    indexMapArea[1] = 9;
    indexMapArea[2] = 17;
    indexMapArea[3] = 33;
    indexMapArea[4] = 65;
    indexMapArea[5] = 129;

    if (this->simpleDemo) {
        this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        this->myLevel = 1;
    } else {
        this->createCombobox();
        this->createAreaLevelCombobox();
    }
}

MainWidget::~MainWidget() {
    cleanup();
}

QSize MainWidget::sizeHint() const {
    if (this->simpleDemo) {
        return {14, 10};
    } else {
        return {1280, 720};
    }
}

void MainWidget::createCombobox() {
    this->myCombobox = new QComboBox;
    this->myCombobox->setParent(this);

    QStringList longerList = (QStringList() <<
            QString("Unigine01") <<
            QString("Unigine02") <<
            QString("Unigine03") <<
            QString("Unigine04") <<
            QString("Unigine05") <<
            QString("Unigine06") <<
            QString("Unigine07"));
    this->myCombobox->addItems(longerList);
    this->myCombobox->setFixedSize(100, 20);
    this->myCombobox->setGeometry(10, 10, this->myCombobox->width(), this->myCombobox->height());

    QObject::connect(this->myCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(handlePictureChange(int)));
}

void MainWidget::createAreaLevelCombobox() {
    this->myAreaLevel = new QComboBox;
    this->myAreaLevel->setParent(this);

    QStringList longerList = (QStringList() <<
            QString("0 Level") <<
            QString("1 Level") <<
            QString("2 Level") <<
            QString("3 Level") <<
            QString("4 Level") <<
            QString("5 Level"));
    this->myAreaLevel->addItems(longerList);
    this->myAreaLevel->setFixedSize(80, 20);
    this->myAreaLevel->setGeometry(120, 10, this->myAreaLevel->width(), this->myAreaLevel->height());

    QObject::connect(this->myAreaLevel, SIGNAL(currentIndexChanged(int)), this, SLOT(handleChangeAreaLevel(int)));
}

void MainWidget::handlePictureChange(int index) {
    delete this->srcTexture;
    this->srcTexture = nullptr;

    this->srcTexture = new QOpenGLTexture(QImage(
            QString("src/26_MLAAExample/Textures/Media/Unigine0") +
            QString::number(index + 1) +
            QString(".png")).mirrored());
    update();
}

void MainWidget::handleChangeAreaLevel(int index) {
    delete this->areaTexture;
    this->areaTexture = nullptr;

    this->myLevel = index;

    this->areaTexture = new QOpenGLTexture(QImage(
            QString("src/26_MLAAExample/Textures/AreaMaps/AreaMap") +
            QString::number(int(indexMapArea[index])) +
            QString(".tiff")).mirrored());
    update();
}

void MainWidget::initializeGL() {
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &MainWidget::cleanup);
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Shaders
    this->initShaders();

    // Textures
    this->initTexture();

    // Buffer Texture
    this->generateBufferTexture();

    // Geometry
    this->initGeometry();

    // Buffers
    this->initBuffers();

    this->glSetting();
}

void MainWidget::paintGL() {
    if (this->myLevel > 0) {
        // Pass - 1 Edge Detection
        this->edgeRT->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        SHADER(1)->bind();
        SHADER(1)->setUniformValue("resolution", QVector2D(1.0f / width(), 1.0f / height()));

        glActiveTexture(GL_TEXTURE0);
        SHADER(1)->setUniformValue("map", 0);
        this->srcTexture->bind();

        gridGeometry->drawGeometry(SHADER(1));

        // Pass - 2 Blend Weight
        this->blendWeightRT->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        SHADER(2)->bind();
        SHADER(2)->setUniformValue("resolution", QVector2D(1.0f / width(), 1.0f / height()));

        SHADER(2)->setUniformValue("MAXSEARCHSTEPS", indexMapArea[myLevel]);

        glActiveTexture(GL_TEXTURE0);
        SHADER(2)->setUniformValue("edgesTex", 0);
        auto *myEdgeTexture = new QOpenGLTexture(this->edgeRT->toImage().mirrored());
        myEdgeTexture->bind();

        glActiveTexture(GL_TEXTURE1);
        SHADER(2)->setUniformValue("areaTex", 1);
        this->areaTexture->bind();

        gridGeometry->drawGeometry(SHADER(2));

        // After render, get texture from attachment
        this->getBufferTexture();

        // Pass - 3
        this->neighborhoodBlendingRT->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        SHADER(3)->bind();
        SHADER(3)->setUniformValue("resolution", QVector2D(1.0f / width(), 1.0f / height()));

        glActiveTexture(GL_TEXTURE0);
        SHADER(3)->setUniformValue("blendTex", 0);
        glBindTexture(GL_TEXTURE_2D, bufferTextures[0]);

        glActiveTexture(GL_TEXTURE1);
        SHADER(3)->setUniformValue("colorTex", 1);
        this->srcTexture->bind();

        glActiveTexture(GL_TEXTURE2);
        SHADER(3)->setUniformValue("alphaTex", 2);
        glBindTexture(GL_TEXTURE_2D, bufferTextures[1]);

        gridGeometry->drawGeometry(SHADER(3));
    }

    // ----- Go back to default buffer ----- //
    QOpenGLFramebufferObject::bindDefault();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    SHADER(0)->bind();

    glActiveTexture(GL_TEXTURE0);
    SHADER(0)->setUniformValue("map", 0);

    if (this->myLevel > 0) {
        auto *myNeighborhoodBlendingTexture = new QOpenGLTexture(this->neighborhoodBlendingRT->toImage().mirrored());
        myNeighborhoodBlendingTexture->bind();
    } else {
        this->srcTexture->bind();
    }

    gridGeometry->drawGeometry(SHADER(0));
}

void MainWidget::initShaders() {
    // Screen Quad Shader
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/26_MLAAExample/Shaders/ScreenPlane.vs.glsl"))
        close();
    if (!SHADER(0)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/26_MLAAExample/Shaders/ScreenPlane.fs.glsl"))
        close();
    if (!SHADER(0)->link())
        close();

    // Pass - 1 Edge Detection Shader
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/26_MLAAExample/Shaders/MLAA_EdgeDetectionPS.vs.glsl"))
        close();
    if (!SHADER(1)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/26_MLAAExample/Shaders/MLAA_EdgeDetectionPS.fs.glsl"))
        close();
    if (!SHADER(1)->link())
        close();

    // Pass - 2 Blend Weight
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/26_MLAAExample/Shaders/MLAA_BlendWeightPS.vs.glsl"))
        close();
    if (!SHADER(2)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/26_MLAAExample/Shaders/MLAA_BlendWeightPS.fs.glsl"))
        close();
    if (!SHADER(2)->link())
        close();

    // Pass - 3 Neighborhood Blending
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Vertex, "src/26_MLAAExample/Shaders/MLAA_NeighborhoodBlendingPS.vs.glsl"))
        close();
    if (!SHADER(3)->addShaderFromSourceFile(QOpenGLShader::Fragment, "src/26_MLAAExample/Shaders/MLAA_NeighborhoodBlendingPS.fs.glsl"))
        close();
    if (!SHADER(3)->link())
        close();
}

void MainWidget::initGeometry() {
    gridGeometry = new GridGeometry;
    gridGeometry->initGeometry();

    gridGeometry->setupAttributePointer(SHADER(0));
    gridGeometry->setupAttributePointer(SHADER(1));
    gridGeometry->setupAttributePointer(SHADER(2));
    gridGeometry->setupAttributePointer(SHADER(3));
}

void MainWidget::initTexture() {
    if (this->simpleDemo) {
        this->srcTexture = new QOpenGLTexture(QImage(QString("src/26_MLAAExample/Textures/MLAA_Source.png")).mirrored());
    } else {
        this->srcTexture = new QOpenGLTexture(QImage(QString("src/26_MLAAExample/Textures/Media/Unigine01.png")).mirrored());
    }
    this->areaTexture = new QOpenGLTexture(QImage(QString("src/26_MLAAExample/Textures/AreaMaps/AreaMap9.tiff")).mirrored());
}

void MainWidget::initBuffers() {
    QOpenGLFramebufferObjectFormat format;
    QSize frameBufferSize(width() * devicePixelRatio(), height() * devicePixelRatio());

    // edge detection
    this->edgeRT = new QOpenGLFramebufferObject(frameBufferSize, format);

    // neighborhood blending
    this->neighborhoodBlendingRT = new QOpenGLFramebufferObject(frameBufferSize, format);

    // blend weight
    this->blendWeightRT = new QOpenGLFramebufferObject(frameBufferSize, format);
    this->blendWeightRT->bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->blendWeightBufferTexture->textureId(), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->blendWeightAlphaBufferTexture->textureId(), 0);
    // blend weight alpha

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
}

void MainWidget::generateBufferTexture() {
    // ----- blend weight texture ----- //
    blendWeightBufferTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);

    blendWeightBufferTexture->create();
    blendWeightBufferTexture->setFormat(QOpenGLTexture::RGB16F);
    blendWeightBufferTexture->setSize(this->width(), this->height(), 1);

    // 分配内存
    blendWeightBufferTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::Float16);

    // ----- blend weight alpha texture ----- //
    blendWeightAlphaBufferTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);

    blendWeightAlphaBufferTexture->create();
    blendWeightAlphaBufferTexture->setFormat(QOpenGLTexture::RGB16F);
    blendWeightAlphaBufferTexture->setSize(this->width(), this->height(), 1);

    // 分配内存
    blendWeightAlphaBufferTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::Float16);
}

void MainWidget::getBufferTexture() {
    bufferTextures.clear();
    bufferTextures.resize(2);

    QScopedPointer<QOpenGLFramebufferObject> tmpFBO;

    for (int i = 0 ; i < 2; i++) {
        tmpFBO.reset(new QOpenGLFramebufferObject(blendWeightRT->size()));
        QOpenGLFramebufferObject::blitFramebuffer(
                tmpFBO.data(),
                QRect(0, 0, width(), height()),
                blendWeightRT,
                QRect(0, 0, width(), height()),
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST,
                i,
                0);
        // save to disk
        if (i == 0) {
            tmpFBO->toImage().save("src/26_MLAAExample/RenderOut/blendWeight.png");
        } else if (i == 1) {
            tmpFBO->toImage().save("src/26_MLAAExample/RenderOut/blendWeightAlpha.png");
        }

        bufferTextures[i] = tmpFBO->takeTexture(0);
    }
}

void MainWidget::glSetting() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void MainWidget::cleanup() {
    makeCurrent();

    // delete programs
    qDeleteAll(programs);
    programs.clear();

    bufferTextures.clear();

    delete gridGeometry;
    delete srcTexture;
    delete areaTexture;
    delete edgeRT;
    delete blendWeightRT;
    delete neighborhoodBlendingRT;

    gridGeometry = nullptr;
    srcTexture = nullptr;
    areaTexture = nullptr;
    edgeRT = nullptr;
    blendWeightRT = nullptr;
    neighborhoodBlendingRT = nullptr;
    blendWeightBufferTexture = nullptr;
    blendWeightAlphaBufferTexture = nullptr;

    doneCurrent();
}

void MainWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        this->close();
    } else if (event->key() == Qt::Key_S) {
        QImage saveImage = this->grab().toImage().convertToFormat(QImage::Format_RGBA8888);
        saveImage.save("src/26_MLAAExample/RenderOut/Debug.png");
    }

    QWidget::keyPressEvent(event);
}
