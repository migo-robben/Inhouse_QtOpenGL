set(TARGET_NAME QtOpenGLModel)
set(SOURCES main.cpp hellowindow.cpp ../Helper/Camera.cpp ../Helper/CustomGeometry.cpp)
qt_add_resources(SOURCES application.qrc)

# https://doc.qt.io/qt-6/qt-add-resources.html
add_executable(${TARGET_NAME}
        ${SOURCES})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")
include_directories("E:/DEV_PROJECTS/WSRD/assimp/Release/include")
target_link_libraries(${TARGET_NAME} PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)

if (WIN32)
    target_link_libraries(${TARGET_NAME} PUBLIC
            "E:/DEV_PROJECTS/WSRD/assimp/Release/lib/assimp-vc142-mt.lib")
endif()