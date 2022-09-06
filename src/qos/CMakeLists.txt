cmake_minimum_required(VERSION 3.13)

add_library(qos INTERFACE)

target_sources(qos INTERFACE
  atomic.S
  c.c
  dlist.cpp
  event.cpp
  interrupt.cpp
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
)

target_include_directories(qos INTERFACE ..)

target_link_libraries(qos INTERFACE
  hardware_exception
  hardware_irq
  hardware_timer
  hardware_uart
  pico_multicore
)

add_library(pico_divider_qos INTERFACE)

pico_wrap_function(pico_divider_qos __aeabi_idiv)
pico_wrap_function(pico_divider_qos __aeabi_idivmod)
pico_wrap_function(pico_divider_qos __aeabi_uidiv)
pico_wrap_function(pico_divider_qos __aeabi_uidivmod)
