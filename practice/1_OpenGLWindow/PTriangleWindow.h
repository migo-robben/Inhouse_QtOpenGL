#ifndef QTOPENGLREFERENCE_PTRIANGLEWINDOW_H
#define QTOPENGLREFERENCE_PTRIANGLEWINDOW_H
#include "POpenGLWindow.h"
#include <QOpenGLShaderProgram>

static const char *vertexShaderSource =
        "#version 460 core\n"
        "layout (location=0) in vec3 posAttr;\n"
        "layout (location=1) in vec3 colAttr;\n"
        "out vec3 col;\n"
        "uniform mat4 matrix;\n"
        "void main() {\n"
        "   col = colAttr;\n"
        "   gl_Position = matrix * vec4(posAttr.x, posAttr.y, posAttr.z, 1.0);\n"
        "}\n";

static const char *fragmentShaderSource =
        "#version 460 core\n"
        "in vec3 col;\n"
        "out vec4 FragColor;"
        "void main() {\n"
        "   FragColor = vec4(col, 1.0);\n"
        "}\n";

class PTriangleWindow: public POpenGLWindow {

public:
    void render() override;
    void initialize() override;

private:
    GLuint m_posAttr{};
    GLuint m_colAttr{};
    GLuint m_matrixUniform{};
    QOpenGLShaderProgram *m_program = nullptr;
    int m_frame = 0;
    QMatrix4x4 matrix;
    const qreal zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
};


#endif //QTOPENGLREFERENCE_PTRIANGLEWINDOW_H
