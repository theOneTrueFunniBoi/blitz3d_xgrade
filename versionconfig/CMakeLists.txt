cmake_minimum_required(VERSION 3.16)

project(versionconfig)

add_library(versionconfig
	versionconfig.cpp
	versionconfig.h
)

target_compile_options(versionconfig PRIVATE /Gz)
