cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)

pico_sdk_init()

project(qos C CXX ASM)

add_subdirectory(examples)
add_subdirectory(src/qos)

