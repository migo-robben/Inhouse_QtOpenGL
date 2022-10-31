set(TARGET_NAME QtOpenGLTexture)
set(SOURCES main.cpp hellowindow.cpp)
qt_add_resources(SOURCES application.qrc)

# https://doc.qt.io/qt-6/qt-add-resources.html
add_executable(${TARGET_NAME}
        ${SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)
