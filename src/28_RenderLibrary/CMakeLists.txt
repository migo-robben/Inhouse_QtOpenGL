set(TARGET_NAME MurrayRenderer)

# 将 MurrayRenderer 编译成静态库/动态库
set(BUILD_SHARED_LIBS ON)

add_library(${TARGET_NAME} MurrayRenderer.cpp)

target_link_libraries(${TARGET_NAME} Qt6::Core)
target_link_libraries(${TARGET_NAME} Qt6::Gui)
target_link_libraries(${TARGET_NAME} Qt6::OpenGL)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})

# UnitTest
add_subdirectory(UnitTest)

# Wrapper
add_subdirectory(Wrapper)
