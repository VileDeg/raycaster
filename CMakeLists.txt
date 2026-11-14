cmake_minimum_required(VERSION 3.22)

# Allow the user to override the build type if they want Release/RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(Raycaster C)

# Enforce a modern C standard (C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MYEXEC rayc)
set(SRC_DIR ./src)
set(SOURCES 
    ${SRC_DIR}/main.c 
    ${SRC_DIR}/glad.c 
    ${SRC_DIR}/vec2.h 
    ${SRC_DIR}/texture_buffer.h 
    ${SRC_DIR}/game_map.h
)

add_executable(${MYEXEC} ${SOURCES})

# --- HARDENING & WARNINGS ---
if(MSVC)
    target_compile_options(${MYEXEC} PRIVATE 
        /W4     # High warning level
        /WX     # Treat warnings as errors
        /wd4100 # Unreferenced formal parameter (common in window/input callbacks)
        /wd4201 # Nonstandard extension used: nameless struct/union (common in math types)
        /wd4244 # Allow implicit conversion loss of data
		/wd4459 # Hides global declaration
    )
else()
    target_compile_options(${MYEXEC} PRIVATE 
        -Wall 
        -Wextra 
        -Werror                 # Treat warnings as errors
        -Wno-unused-parameter   # Ignored because window/input callbacks often require unused arguments
    )
endif()


# GLFW configuration (must remain before add_subdirectory)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_include_directories(${MYEXEC} PRIVATE ${SRC_DIR})

add_subdirectory(./glfw-3.3.8)

target_link_libraries(${MYEXEC} PRIVATE glfw)

find_package(OpenGL REQUIRED)

target_link_libraries(${MYEXEC} PRIVATE OpenGL::GL)