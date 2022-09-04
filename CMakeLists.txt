cmake_minimum_required(VERSION 3.13)

list(APPEND PICO_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/src/qos/pico_sdk_config.h)

include(pico-sdk/pico_sdk_init.cmake)

pico_sdk_init()

project(qos C CXX ASM)

add_subdirectory(examples)
add_subdirectory(src/qos)

