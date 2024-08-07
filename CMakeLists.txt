cmake_minimum_required(VERSION 3.23)
project(Inhouse_QtOpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH
    ${USD_CMAKE_MODULE_PATH}/defaults
    ${USD_CMAKE_MODULE_PATH}/modules
    ${USD_CMAKE_MODULE_PATH}/macros
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PXR_USE_DEBUG_PYTHON ON)
endif()

include(Options)
include(ProjectDefaults)
include(Packages)

include(Public)
add_custom_target(shared_libs)

# Set cxx flags
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

add_definitions(-D NOMINMAX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "CMAKE Debug")
    set(ASSIMP_LIB_FLAG mtd)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "CMAKE Release")
    set(ASSIMP_LIB_FLAG mt)
endif()

# third-party
find_package(spdlog CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Qt6
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6OpenGL REQUIRED)
find_package(Qt6OpenGLWidgets REQUIRED)
find_package(Qt6Quick REQUIRED)
find_package(Qt6QuickControls2 REQUIRED)
find_package(Qt6Qml REQUIRED)
find_package(Qt6QuickWidgets REQUIRED)

# USD
find_package(pxr REQUIRED)

# configuration assimp
#if(DEFINED ASSIMP_ROOT)
#    set(ASSIMP_LIBRARIES ${ASSIMP_ROOT}/lib/assimp-vc142-${ASSIMP_LIB_FLAG}.lib)
#    set(ASSIMP_INCLUDE_DIR "${ASSIMP_ROOT}/include")
#    message(STATUS "Config ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES} ASSIMP_INCLUDE: ${ASSIMP_INCLUDE_DIR}")
#else()
#    find_package(Assimp CONFIG)
#    set(ASSIMP_INCLUDE_DIR "/include")
#    set(ASSIMP_LIBRARIES assimp::assimp)
#    message(STATUS "vcpkg ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES} ASSIMP_INCLUDE: ${ASSIMP_INCLUDE_DIR}")
#endif()

if(${EXAMPLE_VULKAN})
    find_package(Vulkan REQUIRED)
    include_directories(BEFORE SYSTEM "${Vulkan_INCLUDE_DIR}")
endif()

add_subdirectory(src)
if(${PRACTICE})
    add_subdirectory(practice)
endif()
if(${EXAMPLES})
    add_subdirectory(examples)
endif()

find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_SHARED_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

