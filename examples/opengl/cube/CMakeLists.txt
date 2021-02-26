set(TARGET_NAME InHouse_E_Cube)

FILE (GLOB COMPILE_FILES CONFIGURE_DEPENDS *.cpp *.h )
message(STATUS "Find files ${COMPILE_FILES}")
add_executable(${TARGET_NAME} ${COMPILE_FILES})

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})