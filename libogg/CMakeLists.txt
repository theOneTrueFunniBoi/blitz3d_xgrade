cmake_minimum_required(VERSION 3.16)

project(libogg)

add_library(libogg INTERFACE)

target_link_libraries(libogg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/win32/VS2010/Win32/Release/libogg_static.lib)

target_include_directories(libogg INTERFACE include)
