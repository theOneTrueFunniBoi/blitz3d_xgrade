cmake_minimum_required(VERSION 3.16)

project(bblaunch)

add_executable(bblaunch WIN32
        bblaunch.cpp
        bblaunch.ico
        bblaunch.rc
        )

set(LIBS versionconfig)

set_property(TARGET bblaunch PROPERTY CXX_STANDARD 17)
# set_property(TARGET bblaunch PROPERTY C_STANDARD 17)

target_compile_options(bblaunch PRIVATE /Gz)

target_link_libraries(bblaunch ${LIBS})

copy_exe_to_install(bblaunch XGrade-Launcher)
