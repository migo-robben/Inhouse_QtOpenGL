add_executable(HelloWorldWindow main.cpp)

target_link_libraries(HelloWorldWindow PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL)
