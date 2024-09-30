cmake_minimum_required(VERSION 3.16)

project(libvorbis)

add_library(libvorbis INTERFACE)

target_link_libraries(libvorbis INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/win32/VS2010/Win32/Release/libvorbis_static.lib ${CMAKE_CURRENT_SOURCE_DIR}/win32/VS2010/Win32/Release/libvorbisfile_static.lib)

target_include_directories(libvorbis INTERFACE include)
