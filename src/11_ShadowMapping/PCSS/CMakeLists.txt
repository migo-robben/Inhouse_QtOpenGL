set(TARGET_NAME InHouse_QtOpenGLPCSS)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../" "${STB_ROOT}")

add_executable(${TARGET_NAME}
        main.cpp
        mainwidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/FloorGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/RectangleGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/CubeGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/CustomGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Animation.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Animator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Camera.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})