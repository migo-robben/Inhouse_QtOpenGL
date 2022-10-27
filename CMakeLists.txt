cmake_minimum_required(VERSION 3.14)
project(Inhouse_QtOpenGL)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

add_subdirectory(src)