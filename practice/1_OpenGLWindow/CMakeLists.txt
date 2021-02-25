set(TARGET_NAME InHouse_P_QtOpenGLWindow)

add_executable(${TARGET_NAME} main.cpp POpenGLWindow.cpp POpenGLWindow.h PTriangleWindow.cpp PTriangleWindow.h)

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})