cmake_minimum_required(VERSION 3.16)

project(bblaunch)

add_executable(bblaunch WIN32
        bblaunch.cpp
        bblaunch.ico
        bblaunch.rc
        )

set(LIBS versionconfig)

target_compile_options(bblaunch PRIVATE /Gz)

target_link_libraries(bblaunch ${LIBS})

copy_exe_to_install(bblaunch Mavless-Launcher)
