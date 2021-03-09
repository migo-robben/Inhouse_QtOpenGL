set(TARGET_NAME InHouse_QtOpenGLModels)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../" "${ASSIMP_ROOT}/include")

add_executable(${TARGET_NAME}
        main.cpp
        mainwidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CustomGeometry.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
target_link_libraries(${TARGET_NAME} ${ASSIMP_ROOT}/lib/assimp-vc141-mt.lib)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})