set(TARGET_NAME InHouse_P_VAOAndTexture)

FILE (GLOB COMPILE_FILES CONFIGURE_DEPENDS *.cpp *.h )
add_executable(${TARGET_NAME} ${COMPILE_FILES} PPlane.cpp PPlane.h PPlaneWidget.cpp PPlaneWidget.h PTriangleWidget.cpp PTriangleWidget.h)

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})