cmake_minimum_required (VERSION 3.22)

set(CMAKE_BUILD_TYPE Debug)

project (Raycaster)


set(MYEXEC rayc)
set(SRC_DIR ./src)
set(SOURCES ${SRC_DIR}/main.c ${SRC_DIR}/glad.c ${SRC_DIR}/vec2.h ${SRC_DIR}/texture_buffer.h ${SRC_DIR}/game_map.h)

add_executable(${MYEXEC} ${SOURCES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_include_directories(${MYEXEC} PRIVATE ${SRC_DIR})




add_subdirectory(./glfw-3.3.8)

target_link_libraries(${MYEXEC} glfw)

find_package(OpenGL REQUIRED)

target_link_libraries(${MYEXEC} OpenGL::GL)