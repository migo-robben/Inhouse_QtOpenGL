#include "openglwindow.h"

#include <QScreen>
#include <QMatrix4x4>
#include <QApplication>
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

class TriangleWindow: public OpenGLWindow
{
public:
    TriangleWindow();

    void initialize() override;
    void render() override;

private:
    GLuint m_posAttr{};
    GLuint m_colAttr{};
    GLuint m_matrixUniform{};

    QOpenGLShaderProgram *m_program;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setSamples(16);

    TriangleWindow window;
    window.setFormat(format);
    window.resize(640, 480);
    window.show();

    return QApplication::exec();
}

TriangleWindow::TriangleWindow(): m_program(nullptr) {
}

void TriangleWindow::initialize() {
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");
}

void TriangleWindow::render() {
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    m_program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60.0f, 4.0f/3.0f, 0.1f, 100.0f);
    matrix.translate(0, 0, -2);
    matrix.rotate(0, 0, 1, 0);

    m_program->setUniformValue(m_matrixUniform, matrix);

    GLfloat vertices[] = {
            0.0f, 0.707f,
            -0.5f, -0.5f,
            0.5f, -0.5f
    };

    GLfloat colors[] = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
    };

    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    m_program->release();
}