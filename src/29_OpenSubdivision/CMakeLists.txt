set(TARGET_NAME InHouse_Osd_Cube)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")

add_executable(${TARGET_NAME}
    main.cpp
    viewer.cpp
#    shape.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../Helper/Camera.cpp"
)

target_include_directories(${TARGET_NAME} PUBLIC "${OPENSUBDIV_INCLUDE_DIR}")
target_link_libraries(${TARGET_NAME} ${OPENSUBDIV_OSDGPU_LIBRARY} ${OPENSUBDIV_OSDCPU_LIBRARY})

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} Qt6::OpenGL)
target_link_libraries(${TARGET_NAME} Qt6::OpenGLWidgets)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})
