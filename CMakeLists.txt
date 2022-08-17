cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)

project(rtos C CXX ASM)

pico_sdk_init()

add_executable(rtos
  atomic.s
  scheduler.s
  scheduler.cpp
  test.cpp
)

pico_add_extra_outputs(rtos)

pico_enable_stdio_usb(rtos 1)

target_link_libraries(rtos
  hardware_dma
  hardware_exception
  pico_stdlib
)
