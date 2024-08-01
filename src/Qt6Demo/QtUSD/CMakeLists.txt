set(TARGET_NAME QtUSD)

include_directories(${PXR_INCLUDE_DIRS})

add_executable(${TARGET_NAME} main.cpp)

target_link_libraries(${TARGET_NAME} ${PXR_LIBRARIES})
