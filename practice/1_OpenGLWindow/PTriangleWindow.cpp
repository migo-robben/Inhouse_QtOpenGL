#include "PTriangleWindow.h"

void PTriangleWindow::render() {
    // POpenGLWindow::render();
    qDebug() << "render frame: " << m_frame;
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT);

    m_program->bind();
    matrix.setToIdentity();

    matrix.perspective(fov, width()>height() ? qreal(width())/qreal(height()) : qreal(height())/qreal(width()), zNear, zFar);
    matrix.translate(0, 0, -2);
    matrix.rotate(100.0f*m_frame / screen()->refreshRate(), 0, 1, 0);


    m_program->setUniformValue(m_matrixUniform, matrix);

    GLfloat vertices[] = {
            0.0f, 0.5f,
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

    ++m_frame;
}

void PTriangleWindow::initialize() {
    // POpenGLWindow::initialize();

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_program->bind();
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");
    m_program->release();
}
