cmake_minimum_required(VERSION 3.10)
project(Inhouse_QtOpenGL)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(${EXAMPLE_VULKAN})
    find_package(Vulkan REQUIRED)
    include_directories(BEFORE SYSTEM "${Vulkan_INCLUDE_DIRS}")
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_subdirectory(src)
#add_subdirectory(practice)
#add_subdirectory(examples)
