set(TARGET_NAME PRTLIB)
include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ASSIMP_ROOT}/include"
        "${STB_ROOT}"
        "${EIGEN_ROOT}/include/eigen3")

add_library(
        ${TARGET_NAME}
        Lighting.cpp
        Sampler.cpp
        Object.cpp
        DiffuseObject.cpp
        BVHTree.cpp
        BoundingBox.cpp
        SHRotation.cpp)

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui)
target_link_libraries(${TARGET_NAME} ASSIMP_LIBRARIES)

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/lib")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})