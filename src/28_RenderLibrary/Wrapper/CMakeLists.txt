set(TARGET_NAME MurrayRendererApi)

add_library(${TARGET_NAME} SHARED MurrayRendererApi.cpp)


target_include_directories(${TARGET_NAME}
    PUBLIC
    ${Boost_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../"
)
target_link_libraries(${TARGET_NAME}
	Qt6::Core
	Qt6::Gui
	Qt6::OpenGL
    ${Boost_PYTHON_LIBRARY}
    ${PYTHON_LIBRARIES}
    MurrayRenderer
)

# Rename the library to MurrayRendererApi.pyd on Windows
set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".pyd")

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})
