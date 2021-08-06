set(TARGET_NAME InHouse_SkeletalAnimation)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../" "${STB_ROOT}" "${ASSIMP_INCLUDE_DIR}")

add_executable(${TARGET_NAME}
        main.cpp
        GLWidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/CustomGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Animation.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Animator.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
target_link_libraries(${TARGET_NAME} ${ASSIMP_LIBRARIES})

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})