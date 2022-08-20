cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)

project(rtos C CXX ASM)

pico_sdk_init()

add_executable(rtos
  atomic.s
  c.c
  critical.inl.c
  critical.s
  mutex.cpp
  queue.cpp
  scheduler.cpp
  scheduler.inl.c
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
  hardware_irq
  hardware_timer
  pico_stdlib
)
