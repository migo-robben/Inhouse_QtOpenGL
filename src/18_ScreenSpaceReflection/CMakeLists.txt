set(TARGET_NAME InHouse_QtOpenGLSSR)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../" "${ASSIMP_ROOT}/include" "${STB_ROOT}")

add_executable(${TARGET_NAME}
        main.cpp
        GLWidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CubeGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CustomGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/SkyboxGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/RectangleGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/SphereGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp")

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} ASSIMP_LIBRARIES)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})