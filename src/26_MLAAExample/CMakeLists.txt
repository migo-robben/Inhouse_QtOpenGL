set(TARGET_NAME InHouse_QtOpenGLMLAA)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../" "${ASSIMP_ROOT}/include" "${STB_ROOT}")

add_executable(${TARGET_NAME}
        main.cpp
        GLWidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/GridGeometry.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})