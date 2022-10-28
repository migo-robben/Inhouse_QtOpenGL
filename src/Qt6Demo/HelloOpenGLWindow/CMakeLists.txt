set(TARGET_NAME HelloOpenGLWindow)
add_executable(${TARGET_NAME}
        main.cpp
        hellowindow.cpp)

target_link_libraries(${TARGET_NAME} PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)
