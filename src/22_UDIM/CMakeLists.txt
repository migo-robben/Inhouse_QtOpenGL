set(TARGET_NAME InHouse_UDIM)

#Inhouse_QtOpenGL/src/22_UDIM
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../")


add_executable(${TARGET_NAME} main.cpp)


#Link python directory, because get error "fatal error LNK1104: cannot open file 'python37.lib'"
target_include_directories(${TARGET_NAME}
        PUBLIC
        ${Python3_INCLUDE_DIR}
        ${USD_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS})

target_link_directories(${TARGET_NAME}
        PUBLIC
        ${Python3_LIBRARY_DIR})

target_link_libraries(${TARGET_NAME}
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        ${Python3_LIBRARY}
        ${USD_LIBRARIES}
        ${Boost_LIBRARIES}
        ${TBB_LIBRARIES})


set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})


