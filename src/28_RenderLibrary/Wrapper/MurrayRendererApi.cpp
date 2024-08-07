//
// Created by wuguo on 8/5/2024.
//
#include <iostream>
#include <boost/python.hpp>
#include "MurrayRenderer.h"

std::string initApi()
{
    return "Init MurrayRendererApi";
}

class WrapperMurrayRenderer {
public:
    WrapperMurrayRenderer()
    {
        murrayRenderer = new MurrayRenderer();
    }

    ~WrapperMurrayRenderer()
    {
        delete murrayRenderer;
    }

    void setWidth(int w)
    {
        murrayRenderer->setWidth(w);
    }

    void setHeight(int h)
    {
        murrayRenderer->setHeight(h);
    }

    void initialize()
    {
        murrayRenderer->initialize();
    }

    void render()
    {
        murrayRenderer->render();
    }

private:
    MurrayRenderer *murrayRenderer;
};

BOOST_PYTHON_MODULE(MurrayRendererApi)
{
    using namespace boost::python;
    def("initApi", initApi);

    class_<ContextHandler>("ContextHandler", init<>())
        .def("makeCurrent", &ContextHandler::makeCurrent)
        .def("doneCurrent", &ContextHandler::doneCurrent)
        .def("create", &ContextHandler::create);

    class_<WrapperMurrayRenderer>("WrapperMurrayRenderer", init<>())
        .def("setWidth", &WrapperMurrayRenderer::setWidth)
        .def("setHeight", &WrapperMurrayRenderer::setHeight)
        .def("initialize", &WrapperMurrayRenderer::initialize)
        .def("render", &WrapperMurrayRenderer::render);
}
