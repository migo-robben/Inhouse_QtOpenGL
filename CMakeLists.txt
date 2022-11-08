cmake_minimum_required(VERSION 3.14)
project(Inhouse_QtOpenGL)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL)
set(CMAKE_AUTOMOC ON)

#find_package(pxr)

add_subdirectory(src)