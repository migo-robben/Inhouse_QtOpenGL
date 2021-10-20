set(TARGET_NAME QML_ScreenshotTool)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")

add_executable(${TARGET_NAME}
        main.cpp
        CaptureWindow.cpp)

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
target_link_libraries(${TARGET_NAME} Qt5::Quick)
target_link_libraries(${TARGET_NAME} Qt5::QuickControls2)
target_link_libraries(${TARGET_NAME} Qt5::Qml)
target_link_libraries(${TARGET_NAME} Qt5::QuickWidgets)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})