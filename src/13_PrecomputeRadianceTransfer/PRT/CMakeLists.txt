set(TARGET_NAME PRT)
include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/../"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../"
        "${ASSIMP_ROOT}/include"
        "${STB_ROOT}")

add_executable(${TARGET_NAME}
        main.cpp
        mainwidget.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Geometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/CustomGeometry.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Helper/Camera.cpp")

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
if(${ASSIMP_ROOT})
    target_link_libraries(${TARGET_NAME} ${ASSIMP_ROOT}/lib/assimp-vc141-mt.lib)
else()
    target_link_libraries(${TARGET_NAME} assimp::assimp)
endif()
target_link_libraries(${TARGET_NAME} PRTLIB)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})
