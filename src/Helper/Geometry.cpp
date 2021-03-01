#include "Geometry.h"

Geometry::Geometry()
        : vbo(QOpenGLBuffer::VertexBuffer),
        ebo(QOpenGLBuffer::IndexBuffer)
{
    QOpenGLFunctions_4_5_Core::initializeOpenGLFunctions();

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    vbo.create();
    ebo.create();
}

Geometry::~Geometry()
{
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
}