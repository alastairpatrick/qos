cmake_minimum_required(VERSION 3.13)

list(APPEND PICO_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_config.h)

include(pico-sdk/pico_sdk_init.cmake)

project(rtos C CXX ASM)

pico_sdk_init()

add_executable(rtos
  atomic.s
  c.c
  critical.s
  dlist.cpp
  interrupt.cpp
  interrupt.s
  live_core.cpp
  mutex.cpp
  queue.cpp
  scheduler.cpp
  scheduler.s
  semaphore.cpp
  test.c
)

pico_add_extra_outputs(rtos)

pico_enable_stdio_usb(rtos 1)

target_link_libraries(rtos
  hardware_claim
  hardware_dma
  hardware_exception
  hardware_gpio
  hardware_irq
  hardware_pwm
  hardware_timer
  pico_multicore
  pico_stdlib
  pico_time
  pico_sync
)
