cmake_minimum_required(VERSION 3.16)

project(config)

add_library(config
        config.cpp
        config.h
)
set(LIBS versionconfig)
target_link_libraries(config ${LIBS})

target_compile_options(config PRIVATE /Gz)
