set(TARGET_NAME InHouse_QtOpenGLVAOAndTexture)

add_executable(${TARGET_NAME} main.cpp mainwidget.cpp)

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} Qt6::OpenGL)
target_link_libraries(${TARGET_NAME} Qt6::OpenGLWidgets)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})