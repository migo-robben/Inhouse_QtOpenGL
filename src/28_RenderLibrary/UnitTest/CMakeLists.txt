set(TARGET_NAME testMurrayRender)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Widgets)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} MurrayRenderer)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})