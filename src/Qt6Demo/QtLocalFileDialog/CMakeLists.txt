set(TARGET_NAME QtWebassemblyFileDialog)
set(SOURCES main.cpp)
qt_add_resources(SOURCES application.qrc)

add_executable(${TARGET_NAME}
        ${SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets)
