cmake_minimum_required(VERSION 3.13)

add_executable(qos_test
  test.c
)

pico_set_binary_type(qos_test copy_to_ram)

target_link_libraries(qos_test
  hardware_gpio
  hardware_irq
  hardware_pwm
  hardware_timer
  hardware_uart
  pico_stdlib
  pico_time
  qos
)

pico_add_extra_outputs(qos_test)

pico_enable_stdio_usb(qos_test 0)
pico_enable_stdio_uart(qos_test 0)

pico_set_float_implementation(qos_test compiler)
pico_set_double_implementation(qos_test compiler)
pico_set_divider_implementation(qos_test qos)

target_compile_definitions(qos_test PRIVATE
  PICO_STACK_SIZE=2048
  QOS_PROTECT_CORE0_FLASH=1
  QOS_PROTECT_CORE1_FLASH=1  
)
