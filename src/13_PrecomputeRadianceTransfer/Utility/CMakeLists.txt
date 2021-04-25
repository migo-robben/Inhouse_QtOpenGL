set(TARGET_NAME PRTLIB)
include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ASSIMP_ROOT}/include"
        "${STB_ROOT}")

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
if(${ASSIMP_ROOT})
    target_link_libraries(${TARGET_NAME} ${ASSIMP_ROOT}/lib/assimp-vc141-mt.lib)
else()
    target_link_libraries(${TARGET_NAME} assimp::assimp)
endif()

set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/lib")
install (TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})