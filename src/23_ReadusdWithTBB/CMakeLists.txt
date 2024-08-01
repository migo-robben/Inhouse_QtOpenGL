set(TARGET_NAME ReadPixarUSD)

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
set(TBB_LIBRARIES
        ${USD_HOME}/lib/tbb.lib
        ${USD_HOME}/lib/tbb_debug.lib)

# USD - Python
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
set(PXR_USE_PYTHON_3 ON)

# USD - Boost
add_definitions(-D BOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_LIB_PREFIX "")
set(BOOST_ROOT ${USD_HOME})
set(Boost_INCLUDE_DIRS ${USD_HOME}/include/boost-1_70)
set(Boost_LIBRARY_DIRS ${USD_HOME}/lib)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D TBB_USE_DEBUG=0)
    set(Boost_LIBRARIES
            #${Boost_LIBRARY_DIRS}/boost_log-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_python37-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_date_time-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_filesystem-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_thread-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_regex-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_chrono-vc142-mt-gd-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_atomic-vc142-mt-gd-x64-1_70.lib)
else()
    set(Boost_LIBRARIES
            #${Boost_LIBRARY_DIRS}/boost_log-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_python37-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_date_time-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_filesystem-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_thread-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_regex-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_chrono-vc142-mt-x64-1_70.lib
            ${Boost_LIBRARY_DIRS}/boost_atomic-vc142-mt-x64-1_70.lib)

endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../"
        ${Python3_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS})

add_executable(${TARGET_NAME}
        main.cpp
        GLWidget.cpp
        usdParser.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp")

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} ${USD_LIBRARIES} ${Boost_LIBRARIES} ${TBB_LIBRARIES} ${Python3_LIBRARIES})
target_link_libraries(${TARGET_NAME} spdlog::spdlog spdlog::spdlog_header_only)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})