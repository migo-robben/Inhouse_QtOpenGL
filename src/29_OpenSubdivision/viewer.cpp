//
// Created by PC on 9/2/2024.
//

#include "viewer.h"

#include <QKeyEvent>

#include "vertex.h"
#include "simple_math.h"

#include <opensubdiv/far/topologyDescriptor.h>

Viewer::Viewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , vao(nullptr)
    , ebo(nullptr)
    , vbo(nullptr)
{
    newBuffer();
}

Viewer::~Viewer()
{
    makeCurrent();

    vbo->destroy();
    ebo->destroy();
    vao->destroy();

    doneCurrent();
}

void Viewer::newBuffer()
{
    makeCurrent();

    if (vao != nullptr)
    {
        delete vao;
        vao = nullptr;
    }

    if (ebo != nullptr)
    {
        delete ebo;
        ebo = nullptr;
    }

    if (vbo != nullptr)
    {
        delete vbo;
        vbo = nullptr;
    }

    vao = new QOpenGLVertexArrayObject;
    ebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
}

void Viewer::createBuffer()
{
    makeCurrent();

    if (ebo->isCreated())
    {
        ebo->destroy();
        ebo->create();
    } else
    {
        ebo->create();
    }

    if (vbo->isCreated())
    {
        vbo->destroy();
        vbo->create();
    } else
    {
        vbo->create();
    }
}

QString Viewer::GetShaderVersion()
{
    QString shader_version;

    int major, minor;
    GetMajorMinorVersion(&major, &minor);

    switch (int version_number = major * 10 + minor){
    case 20:
        shader_version = "110";
        break;
    case 21:
        shader_version = "120";
        break;
    case 30:
        shader_version = "130";
        break;
    case 31:
        shader_version = "140";
        break;
    case 32:
        shader_version = "150";
        break;
    default:
        shader_version = QString::number(version_number) + "0";
        break;
    }

    return "#version " + shader_version + "\n";
}

void Viewer::GetMajorMinorVersion(int* major, int* minor)
{
    const GLubyte *ver = glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (!ver){
        *major = -1;
        *minor = -1;
    }
    else{
        std::stringstream ss;
        ss << std::string(ver, ver + 1) << " " << std::string(ver + 2, ver + 3);
        ss >> *major;
        ss >> *minor;
    }
}

void Viewer::initializeGL()
{
    initializeOpenGLFunctions();

    initGL();
    rebuildMesh();
}

void Viewer::initGL()
{
    glClearColor(0.25f, 0.25f, 0.25f, 0.0f); // grey
    glEnable(GL_DEPTH_TEST); // enable depth test
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
}

void Viewer::paintGL()
{
    updateGeom();
    display();
}

void Viewer::updateUniformBlocks()
{
    if (! g_transformUB) {
        glGenBuffers(1, &g_transformUB);
        glBindBuffer(GL_UNIFORM_BUFFER, g_transformUB);
        glBufferData(GL_UNIFORM_BUFFER,
                sizeof(g_transformData), NULL, GL_STATIC_DRAW);
    };
    glBindBuffer(GL_UNIFORM_BUFFER, g_transformUB);
    glBufferSubData(GL_UNIFORM_BUFFER,
                0, sizeof(g_transformData), &g_transformData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, g_transformBinding, g_transformUB);

    // Update and bind lighting state
    struct Lighting {
        struct Light {
            float position[4];
            float ambient[4];
            float diffuse[4];
            float specular[4];
        } lightSource[2];
    } lightingData = {
        {{  { 0.5,  0.2f, 1.0f, 0.0f },
            { 0.1f, 0.1f, 0.1f, 1.0f },
            { 0.7f, 0.7f, 0.7f, 1.0f },
            { 0.8f, 0.8f, 0.8f, 1.0f } },

          { { -0.8f, 0.4f, -1.0f, 0.0f },
            {  0.0f, 0.0f,  0.0f, 1.0f },
            {  0.5f, 0.5f,  0.5f, 1.0f },
            {  0.8f, 0.8f,  0.8f, 1.0f } }}
    };
    if (! g_lightingUB) {
        glGenBuffers(1, &g_lightingUB);
        glBindBuffer(GL_UNIFORM_BUFFER, g_lightingUB);
        glBufferData(GL_UNIFORM_BUFFER,
                sizeof(lightingData), nullptr, GL_STATIC_DRAW);
    };
    glBindBuffer(GL_UNIFORM_BUFFER, g_lightingUB);
    glBufferSubData(GL_UNIFORM_BUFFER,
                0, sizeof(lightingData), &lightingData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, g_lightingBinding, g_lightingUB);
}

void Viewer::rebuildMesh()
{
    delete mesh;
    mesh = nullptr;

    constexpr Sdc::SchemeType type = Sdc::SCHEME_CATMARK;
    Sdc::Options options;
    options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

    typedef Far::TopologyDescriptor Descriptor;
    Descriptor desc;
    desc.numVertices         = Geometry::g_nverts;
    desc.numFaces            = Geometry::g_nfaces;
    desc.numVertsPerFace     = Geometry::g_vertsperface;
    desc.vertIndicesPerFace  = Geometry::g_vertIndices;

    Far::TopologyRefiner * refiner =
        Far::TopologyRefinerFactory<Descriptor>::Create(
            desc,
            Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

    Osd::MeshBitset bits;
    bits.set(Osd::MeshAdaptive, adaptive);
    bits.set(Osd::MeshUseSmoothCornerPatch, true);
    bits.set(Osd::MeshUseSingleCreasePatch, true);
    bits.set(Osd::MeshUseInfSharpPatch, true);
    bits.set(Osd::MeshInterleaveVarying, false);
    bits.set(Osd::MeshFVarData, false);
    bits.set(Osd::MeshEndCapBilinearBasis, false);
    bits.set(Osd::MeshEndCapBSplineBasis, false);
    bits.set(Osd::MeshEndCapGregoryBasis, true);
    bits.set(Osd::MeshEndCapLegacyGregory, false);

    int numVertexElements = 3;
    int numVaryingElements = 0;

    mesh = new Osd::Mesh<
        Osd::CpuGLVertexBuffer,
        Far::StencilTable,
        Osd::CpuEvaluator,
        Osd::GLPatchTable>(
            refiner,
            numVertexElements,
            numVaryingElements,
            level,
            bits);

    updateGeom();

    // 细分之后要重新分配内存空间大小
    createBuffer();

    // -------- VAO -------- //
    QOpenGLVertexArrayObject::Binder vaoBinder(vao);

    Osd::CpuPatchTable patchTable(mesh->GetFarPatchTable());

    ebo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    ebo->bind();
    ebo->allocate(patchTable.GetPatchIndexBuffer(), patchTable.GetPatchIndexSize() * sizeof(GLint));

    int numElements = mesh->GetVertexBuffer()->GetNumElements();
    int numVertices = mesh->GetVertexBuffer()->GetNumVertices();

    mesh->GetVertexBuffer()->BindCpuBuffer();

    int size = numElements * numVertices * (int)sizeof(float);
    vbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vbo->bind();
    vbo->allocate(mesh->GetVertexBuffer()->BindCpuBuffer(), size);

    for (const auto & patch : mesh->GetPatchTable()->GetPatchArrays())
    {
        EffectDesc effectDesc(patch.GetDescriptor(), getEffect());
        auto *config = shaderCache.GetDrawConfig(effectDesc, GetShaderVersion());

        if (!config) return ;

        QOpenGLShaderProgram *program = config->GetProgram();
        program->bind();

        int position = program->attributeLocation("position");
        program->enableAttributeArray(position);
        program->setAttributeBuffer(position, GL_FLOAT, 0, 3, sizeof (GLfloat) * 3);

        program->release();
    }
}

void Viewer::updateGeom()
{
    mesh->UpdateVertexBuffer(&Geometry::g_verts[0], 0, Geometry::g_nverts);
    mesh->Refine();
    mesh->Synchronize();
}

GLenum Viewer::bindProgram(Effect effect, Osd::PatchArray const & patch)
{
    EffectDesc effectDesc(patch.GetDescriptor(), effect);
    auto *config = shaderCache.GetDrawConfig(effectDesc, GetShaderVersion());

    if (!config) return 0;

    QOpenGLShaderProgram *program = config->GetProgram();
    program->bind();

    GLuint uboIndex = glGetUniformBlockIndex(program->programId(), "Transform");
    if (uboIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(program->programId(), uboIndex, g_transformBinding);
    }

    uboIndex = glGetUniformBlockIndex(program->programId(),"Lighting");
    if (uboIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(program->programId(), uboIndex, g_lightingBinding);
    }

    // bind standalone uniforms
    int uniformPrimitiveIdBase = program->uniformLocation("PrimitiveIdBase");
    if (uniformPrimitiveIdBase >= 0)
    {
        program->setUniformValue(uniformPrimitiveIdBase, patch.GetPrimitiveIdBase());
    }

    // update uniform
    int uniformDiffuseColor = program->uniformLocation("diffuseColor");
    if (uniformDiffuseColor >= 0)
        program->setUniformValue(uniformDiffuseColor, 0.4f, 0.4f, 0.8f, 1);

    GLenum primType;
    switch(effectDesc.desc.GetType()) {
    case Far::PatchDescriptor::QUADS:
        primType = GL_LINES_ADJACENCY;
        break;
    case Far::PatchDescriptor::TRIANGLES:
        primType = GL_TRIANGLES;
        break;
    default:
        primType = GL_POINTS;
        break;
    }

    return primType;
}

void Viewer::display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width(), height());

    // model view project matrix
    double aspect = width()/(double)height();
    identity(g_transformData.ModelViewMatrix);
    translate(g_transformData.ModelViewMatrix, 0, 0, -5);
    rotate(g_transformData.ModelViewMatrix, angle.y(), 1, 0, 0);
    rotate(g_transformData.ModelViewMatrix, angle.x(), 0, 1, 0);

    translate(g_transformData.ModelViewMatrix,
                  0, 0, 0);
    perspective(g_transformData.ProjectionMatrix,
                45.0f, (float)aspect, 0.1f, 500.0f);
    multMatrix(g_transformData.ModelViewProjectionMatrix,
               g_transformData.ModelViewMatrix,
               g_transformData.ProjectionMatrix);
    inverseMatrix(g_transformData.ModelViewInverseMatrix,
                  g_transformData.ModelViewMatrix);
    // 得到最终的ModelViewProjection Matrix

    updateUniformBlocks();

    QOpenGLVertexArrayObject::Binder vaoBinder(vao);

    Osd::PatchArrayVector const & patches =
        mesh->GetPatchTable()->GetPatchArrays();

    // patch drawing
    for (const auto & patch : patches)
    {
        GLenum primType = bindProgram(getEffect(), patch);
        glDrawElements(
            primType,
            patch.GetNumPatches() * patch.GetDescriptor().GetNumControlVertices(),
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(patch.GetIndexBase() * sizeof(unsigned int)));
    }
}

Effect Viewer::getEffect()
{
    return Effect(
        displayStyle,
        shadingMode,
        screenSpaceTess,
        fractionalSpacing,
        patchCull,
        singleCreasePatch);
}

void Viewer::keyPressEvent(QKeyEvent* event)
{
    // 如果用户是按下了"+"键, level + 1
    if (event->key() == Qt::Key_Plus)
    {
        level++;
        if (level > 6) level = 6;
        rebuildMesh();
        update();
    }
    if (event->key() == Qt::Key_Minus)
    {
        level--;
        if (level < 1) level = 1;
        rebuildMesh();
        update();
    }
    QOpenGLWidget::keyPressEvent(event);
}

void Viewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mousePos = event->pos();
    }
    QOpenGLWidget::mousePressEvent(event);
}

void Viewer::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() == Qt::LeftButton)
    {
        QPoint offset = event->pos() - mousePos;

        angle += QVector2D(offset.x() * 0.25, offset.y() * 0.25);

        update();
        mousePos = event->pos();
    }
    QOpenGLWidget::mouseMoveEvent(event);
}
