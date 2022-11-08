set(TARGET_NAME HelloWorldWindow)

set(SOURCES main.cpp)
qt_add_resources(SOURCES application.qrc)

# https://doc.qt.io/qt-6/qt-add-resources.html
add_executable(${TARGET_NAME}
        ${SOURCES})


target_link_libraries(HelloWorldWindow PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)
