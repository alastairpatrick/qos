cmake_minimum_required(VERSION 3.13)

list(APPEND PICO_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_config.h)

include(pico-sdk/pico_sdk_init.cmake)

project(qos C CXX ASM)

pico_sdk_init()

add_executable(qos
  atomic.S
  c.c
  dlist.cpp
  event.cpp
  interrupt.cpp
  interrupt.S
  lock_core.cpp
  mutex.cpp
  parallel.cpp
  queue.cpp
  spsc_queue.cpp
  svc.S
  semaphore.cpp
  stdio_uart.cpp
  task.cpp
  task.S
  time.cpp
  test.c
)

pico_add_extra_outputs(qos)

pico_enable_stdio_usb(qos 0)
pico_enable_stdio_uart(qos 0)

target_link_libraries(qos
  hardware_claim
  hardware_dma
  hardware_exception
  hardware_gpio
  hardware_irq
  hardware_pwm
  hardware_timer
  hardware_uart
  pico_multicore
  pico_stdlib
  pico_time
  pico_sync
)

add_library(pico_divider_qos INTERFACE
)

pico_wrap_function(pico_divider_qos __aeabi_idiv)
pico_wrap_function(pico_divider_qos __aeabi_idivmod)
pico_wrap_function(pico_divider_qos __aeabi_uidiv)
pico_wrap_function(pico_divider_qos __aeabi_uidivmod)

pico_set_float_implementation(qos compiler)
pico_set_double_implementation(qos compiler)
pico_set_divider_implementation(qos qos)
