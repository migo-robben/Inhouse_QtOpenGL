set(TARGET_NAME QtOpenGLCamera)
set(SOURCES main.cpp hellowindow.cpp ../Helper/Camera.cpp)
qt_add_resources(SOURCES application.qrc)

# https://doc.qt.io/qt-6/qt-add-resources.html
add_executable(${TARGET_NAME}
        ${SOURCES})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${TARGET_NAME} PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)