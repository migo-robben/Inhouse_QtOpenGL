cmake_minimum_required(VERSION 3.17)
project(Inhouse_QtOpenGL)

set(CMAKE_CXX_STANDARD 14)
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

# USD
add_compile_options(/w44244)
add_compile_options(/w44305)
add_compile_options(/w44267)
add_compile_options(/w44506)
add_compile_options(/w44091)
add_compile_options(/w44273)
add_compile_options(/w44180)
add_compile_options(/w44334)

set(USD_ROOT ${USD_HOME})
set(USD_INSTALL_DIR ${USD_HOME})
set(USD_INCLUDE_DIR ${USD_ROOT}/include)
set(USD_LIBRARY_DIR ${USD_ROOT}/lib)


# USD - TBB
add_definitions(-D TBB_USE_ASSERT)
add_definitions(-D TBB_USE_THREADING_TOOLS)
set(TBB_ROOT ${USD_HOME})
set(TBB_INCLUDE_DIRS ${USD_HOME}/include)
set(TBB_LIBRARIES ${USD_HOME}/lib/tbb.lib;${USD_HOME}/lib/tbb_debug.lib)


# USD - Python
set(PXR_USE_PYTHON_3 ON)
set(Python3_ROOT_DIR ${PROJECT_SOURCE_DIR}/lib/thirdparty/Python37)
set(Python3_EXECUTABLE ${Python3_ROOT_DIR}/bin/python.exe)
set(Python3_INCLUDE_DIR ${Python3_ROOT_DIR}/include)
set(Python3_INCLUDE_DIRS ${Python3_ROOT_DIR}/include)
set(Python3_LIBRARY optimized;${Python3_ROOT_DIR}/libs/python37.lib;debug;${Python3_ROOT_DIR}/libs/python37_d.lib)
set(Python3_LIBRARY_DIR ${Python3_ROOT_DIR}/libs)


# USD - Boost
add_definitions(-D BOOST_ALL_DYN_LINK)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_LIB_PREFIX "")
set(BOOST_ROOT ${USD_HOME})
set(Boost_INCLUDE_DIRS ${USD_HOME}/include/boost-1_70)
set(Boost_LIBRARY_DIRS ${USD_HOME}/lib)
set(Boost_LIBRARIES
#        ${Boost_LIBRARY_DIRS}/boost_log-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_python37-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_date_time-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_filesystem-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_thread-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_regex-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_chrono-vc142-mt-gd-x64-1_70.lib;
        ${Boost_LIBRARY_DIRS}/boost_atomic-vc142-mt-gd-x64-1_70.lib)


# Find Package
find_package(spdlog CONFIG REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(USD REQUIRED)
#find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
#find_package(TBB REQUIRED)
#find_package(Boost COMPONENTS python37 log REQUIRED)

# configuration assimp
if(DEFINED ASSIMP_ROOT)
    set(ASSIMP_LIBRARIES ${ASSIMP_ROOT}/lib/assimp-vc142-${ASSIMP_LIB_FLAG}.lib)
    set(ASSIMP_INCLUDE_DIR "${ASSIMP_ROOT}/include")
    message(STATUS "Config ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES} ASSIMP_INCLUDE: ${ASSIMP_INCLUDE_DIR}")
else()
    find_package(Assimp CONFIG)
    set(ASSIMP_INCLUDE_DIR "/include")
    set(ASSIMP_LIBRARIES assimp::assimp)
    message(STATUS "vcpkg ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES} ASSIMP_INCLUDE: ${ASSIMP_INCLUDE_DIR}")
endif()

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

