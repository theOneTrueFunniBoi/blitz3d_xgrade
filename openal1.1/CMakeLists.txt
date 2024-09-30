cmake_minimum_required(VERSION 3.16)

project(openal1.1)

add_library(openal1.1 INTERFACE)

target_link_libraries(openal1.1 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/Win32/OpenAL32.lib)

target_include_directories(openal1.1 INTERFACE include)
