//
// Created by wuguo on 8/5/2024.
//

#include "MurrayRenderer.h"

#include <QOffscreenSurface>
#include <QDebug>

// ContextHandler
ContextHandler::ContextHandler() {
    context = new QOpenGLContext;
    surface = new QOffscreenSurface;
}

ContextHandler::~ContextHandler() {
    clearUp();
}

bool ContextHandler::create() {
    surface->setFormat(QSurfaceFormat::defaultFormat());
    surface->create();

    if (!context->create()) {
        qWarning() << "Failed to create OpenGL context";
        return false;
    }

    return true;
}

void ContextHandler::makeCurrent() {
    context->makeCurrent(surface);
}

void ContextHandler::doneCurrent() {
    context->doneCurrent();
}

void ContextHandler::clearUp() {
    delete context;
    delete surface;
}

// MurrayRendererAPI
MurrayRenderer::MurrayRenderer(QObject *parent)
    : QObject(parent)
    , fbo(nullptr)
    , width(800)
    , height(600)
{
}

MurrayRenderer::~MurrayRenderer() {
    delete fbo;
}

void MurrayRenderer::initialize() {
    initializeOpenGLFunctions();

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    fbo = new QOpenGLFramebufferObject(width, height, format);
}

void MurrayRenderer::render() {
    if (!fbo)
        return;

    fbo->bind();

    glViewport(0, 0, width, height);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    fbo->toImage().save(QString("RenderOut/MurrayRenderer.png"));

    QOpenGLFramebufferObject::bindDefault();
}
