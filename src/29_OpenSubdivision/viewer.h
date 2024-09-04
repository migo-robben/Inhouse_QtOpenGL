//
// Created by PC on 9/2/2024.
//

#ifndef VIEWER_H
#define VIEWER_H

#include <sstream>

#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_4_5_Core>

#include <opensubdiv/osd/glslPatchShaderSource.h>
#include <opensubdiv/osd/glLegacyGregoryPatchTable.h>
#include <opensubdiv/osd/cpuPatchTable.h>
#include <opensubdiv/osd/cpuGLVertexBuffer.h>
#include <opensubdiv/osd/cpuEvaluator.h>
#include <opensubdiv/osd/glMesh.h>

#include "types.h"

static const char *shaderSource(){
    static const char *gen =
#include "shader.gen.h"
        ;
    return gen;
}

using namespace OpenSubdiv;

union Effect {
    Effect(int displayStyle_, int shadingMode_, int screenSpaceTess_,
           int fractionalSpacing_, int patchCull_, int singleCreasePatch_)
        : value(0) {
        displayStyle = displayStyle_;
        shadingMode = shadingMode_;
        screenSpaceTess = screenSpaceTess_;
        fractionalSpacing = fractionalSpacing_;
        patchCull = patchCull_;
        singleCreasePatch = singleCreasePatch_;
    }

    struct {
        unsigned int displayStyle:2;
        unsigned int shadingMode:4;
        unsigned int screenSpaceTess:1;
        unsigned int fractionalSpacing:1;
        unsigned int patchCull:1;
        unsigned int singleCreasePatch:1;
    };
    int value;

    bool operator < (const Effect &e) const {
        return value < e.value;
    }
};

struct EffectDesc {
    EffectDesc(Far::PatchDescriptor desc,
               Effect effect) : desc(desc), effect(effect),
                                maxValence(0), numElements(0) { }

    Far::PatchDescriptor desc;
    Effect effect;
    int maxValence;
    int numElements;

    bool operator < (const EffectDesc &e) const {
        return
            (desc < e.desc || ((desc == e.desc &&
            (maxValence < e.maxValence || ((maxValence == e.maxValence) &&
            (numElements < e.numElements || ((numElements == e.numElements) &&
            (effect < e.effect))))))));
    }
};

class GLDrawConfig : protected QOpenGLFunctions_4_5_Core
{
public:
    explicit GLDrawConfig(QString version)
    : _version(std::move(version))
    , _numShaders(0) {
        hasError = !_program.create();
    }

    ~GLDrawConfig() override {
        if (_program.isLinked()) {
            _program.removeAllShaders();
            _program.release();
        }
    }

    bool CompileAndAttachShader(QOpenGLShader::ShaderType shaderType, const std::string &code)
    {
        const std::string sources = _version.toStdString() + code;

        if (const char *src = sources.c_str(); !_program.addShaderFromSourceCode(shaderType, src)) {
            qDebug() << "Error compiling GLSL shader: " << _program.log();
            return false;
        }

        return true;
    }

    bool Link()
    {
        if (!_program.link())
            return false;

        return true;
    }

    QOpenGLShaderProgram *GetProgram() {
        return &_program;
    }

    bool hasError;
private:
    QOpenGLShaderProgram _program;
    QString _version;
    int _numShaders;
};

class ShaderCache
{
public:
    typedef QMap<EffectDesc, GLDrawConfig*> ConfigMap;

    static GLDrawConfig *CreateDrawConfig(EffectDesc const & effectDesc, QString const & version)
    {
        auto *config = new GLDrawConfig(version);
        if (config->hasError) {
            qDebug() << "Failed to create GLDrawConfig";
            delete config;
            return nullptr;
        }

        Far::PatchDescriptor::Type type = effectDesc.desc.GetType();

        // common defines
        std::stringstream ss;

        if (type == Far::PatchDescriptor::QUADS) {
            ss << "#define PRIM_QUAD\n";
        } else {
            ss << "#define PRIM_TRI\n";
        }

        // OSD tessellation controls
        if (effectDesc.effect.screenSpaceTess) {
            ss << "#define OSD_ENABLE_SCREENSPACE_TESSELLATION\n";
        }
        if (effectDesc.effect.fractionalSpacing) {
            ss << "#define OSD_FRACTIONAL_ODD_SPACING\n";
        }
        if (effectDesc.effect.patchCull) {
            ss << "#define OSD_ENABLE_PATCH_CULL\n";
        }
        if (effectDesc.effect.singleCreasePatch &&
            type == Far::PatchDescriptor::REGULAR) {
            ss << "#define OSD_PATCH_ENABLE_SINGLE_CREASE\n";
        }

        // for legacy gregory
        ss << "#define OSD_MAX_VALENCE " << effectDesc.maxValence << "\n";
        ss << "#define OSD_NUM_ELEMENTS " << effectDesc.numElements << "\n";

        // display styles
        switch (effectDesc.effect.displayStyle) {
            case kDisplayStyleWire:
                ss << "#define GEOMETRY_OUT_WIRE\n";
                break;
            case kDisplayStyleWireOnShaded:
                ss << "#define GEOMETRY_OUT_LINE\n";
                break;
            case kDisplayStyleShaded:
                ss << "#define GEOMETRY_OUT_FILL\n";
                break;
        }

        // shading mode
        switch (effectDesc.effect.shadingMode) {
            case kShadingMaterial:
                ss << "#define SHADING_MATERIAL\n";
                break;
            case kShadingVaryingColor:
                ss << "#define SHADING_VARYING_COLOR\n";
                break;
            case kShadingInterleavedVaryingColor:
                ss << "#define SHADING_VARYING_COLOR\n";
                break;
            case kShadingFaceVaryingColor:
                ss << "#define OSD_FVAR_WIDTH 2\n";
                ss << "#define SHADING_FACEVARYING_COLOR\n";
                if (!effectDesc.desc.IsAdaptive()) {
                    ss << "#define SHADING_FACEVARYING_UNIFORM_SUBDIVISION\n";
                }
                break;
            case kShadingPatchType:
                ss << "#define SHADING_PATCH_TYPE\n";
                break;
            case kShadingPatchDepth:
                ss << "#define SHADING_PATCH_DEPTH\n";
                break;
            case kShadingPatchCoord:
                ss << "#define SHADING_PATCH_COORD\n";
                break;
            case kShadingNormal:
                ss << "#define SHADING_NORMAL\n";
                break;
            default: break;
        }

        if (type != Far::PatchDescriptor::TRIANGLES &&
            type != Far::PatchDescriptor::QUADS) {
            ss << "#define SMOOTH_NORMALS\n";
        }

        // need for patch color-coding : we need these defines in the fragment shader
        if (type == Far::PatchDescriptor::GREGORY) {
            ss << "#define OSD_PATCH_GREGORY\n";
        } else if (type == Far::PatchDescriptor::GREGORY_BOUNDARY) {
            ss << "#define OSD_PATCH_GREGORY_BOUNDARY\n";
        } else if (type == Far::PatchDescriptor::GREGORY_BASIS) {
            ss << "#define OSD_PATCH_GREGORY_BASIS\n";
        } else if (type == Far::PatchDescriptor::LOOP) {
            ss << "#define OSD_PATCH_LOOP\n";
        } else if (type == Far::PatchDescriptor::GREGORY_TRIANGLE) {
            ss << "#define OSD_PATCH_GREGORY_TRIANGLE\n";
        }

        // include osd PatchCommon
        ss << "#define OSD_PATCH_BASIS_GLSL\n";
        ss << Osd::GLSLPatchShaderSource::GetPatchBasisShaderSource();
        ss << Osd::GLSLPatchShaderSource::GetCommonShaderSource();
        std::string common = ss.str();
        ss.str("");

        // vertex shader
        ss << common
           << (effectDesc.desc.IsAdaptive() ? "" : "#define VERTEX_SHADER\n")
           << shaderSource()
           << Osd::GLSLPatchShaderSource::GetVertexShaderSource(type);
        config->CompileAndAttachShader(QOpenGLShader::Vertex, ss.str());
        ss.str("");

        if (effectDesc.desc.IsAdaptive()) {
            // tess control shader
            ss << common
                << shaderSource()
               << Osd::GLSLPatchShaderSource::GetTessControlShaderSource(type);
            config->CompileAndAttachShader(QOpenGLShader::TessellationControl, ss.str());
            ss.str("");

            // tess eval shader
            ss << common
                << shaderSource()
               << Osd::GLSLPatchShaderSource::GetTessEvalShaderSource(type);
            config->CompileAndAttachShader(QOpenGLShader::TessellationEvaluation, ss.str());
            ss.str("");
        }

        // geometry shader
        ss << common
           << "#define GEOMETRY_SHADER\n"
           << shaderSource();
        config->CompileAndAttachShader(QOpenGLShader::Geometry, ss.str());
        ss.str("");

        // fragment shader
        ss << common
           << "#define FRAGMENT_SHADER\n"
           << shaderSource();
        config->CompileAndAttachShader(QOpenGLShader::Fragment, ss.str());
        ss.str("");

        if (!config->Link()) {
            qDebug() << "Failed to link shader program";
            delete config;
            return nullptr;
        }

        return config;
    }

    GLDrawConfig *GetDrawConfig(EffectDesc const & desc, QString const & version)
    {
        if (const ConfigMap::iterator it = _configMap.find(desc); it != _configMap.end()) {
            return it.value();
        }

        GLDrawConfig *config = CreateDrawConfig(desc, version);
        if (config)
            _configMap[desc] = config;

        return config;
    }

private:
    ConfigMap _configMap;
};

class Viewer final : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
public:
    explicit Viewer(QWidget *parent=nullptr);
    ~Viewer() override;

    QString GetShaderVersion();
    void GetMajorMinorVersion(int *major, int *minor);

protected:
    void initializeGL() override;
    void initGL();
    void paintGL() override;

    void initShaders();

    void updateUniformBlocks();

    void rebuildMesh();
    void updateGeom();
    GLenum bindProgram(Effect effect, Osd::PatchArray const & patch);
    void display();
    Effect getEffect();

    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void newBuffer();
    void createBuffer();

private:
    QOpenGLVertexArrayObject *vao;
    QOpenGLBuffer *vbo; // Vertex Buffer
    QOpenGLBuffer *ebo; // Index Buffer

    QPoint mousePos;
    QVector2D angle;

    int kernel = kCPU;
    int level = 2;
    bool adaptive = false;
    Osd::Mesh<
        Osd::CpuGLVertexBuffer,
        Far::StencilTable,
        Osd::CpuEvaluator,
        Osd::GLPatchTable> *mesh = nullptr;

    // effect
    int displayStyle = kDisplayStyleWireOnShaded;
    int shadingMode = kShadingPatchType;
    int screenSpaceTess = 0;
    int fractionalSpacing = 0;
    int patchCull = 0;
    int singleCreasePatch = 1;

    ShaderCache shaderCache;
    Osd::GLLegacyGregoryPatchTable *legacyGregoryPatchTable = nullptr;

    GLuint  g_transformUB = 0,
            g_transformBinding = 0,
            g_tessellationUB = 0,
            g_tessellationBinding = 0,
            g_lightingUB = 0,
            g_lightingBinding = 2;

    struct Transform {
        float ModelViewMatrix[16];
        float ProjectionMatrix[16];
        float ModelViewProjectionMatrix[16];
        float ModelViewInverseMatrix[16];
    } g_transformData;
};

#endif //VIEWER_H
