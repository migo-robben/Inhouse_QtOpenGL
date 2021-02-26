set(TARGET_NAME InHouse_E_VulkanCubes)

FILE (GLOB COMPILE_FILES CONFIGURE_DEPENDS *.cpp *.h )
message(STATUS "Find files ${COMPILE_FILES}")
message(STATUS "Vulkan: ${Vulkan_INCLUDE_DIRS}")
add_executable(${TARGET_NAME} ${COMPILE_FILES})

target_link_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${Vulkan_LIBRARY})

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)


set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})