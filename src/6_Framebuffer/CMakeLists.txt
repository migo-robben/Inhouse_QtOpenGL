set(TARGET_NAME InHouse_QtOpenGLFramebuffer)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")

add_executable(${TARGET_NAME}
        main.cpp
        mainwidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CubeGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/GridGeometry.cpp")

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})