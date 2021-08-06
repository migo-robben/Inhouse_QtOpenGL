set(TARGET_NAME InHouse_QtOpenGLSSAO)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../" "${ASSIMP_ROOT}/include")

add_executable(${TARGET_NAME}
        main.cpp
        GLWidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CustomGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/RectangleGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
target_link_libraries(${TARGET_NAME} ASSIMP_LIBRARIES)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})