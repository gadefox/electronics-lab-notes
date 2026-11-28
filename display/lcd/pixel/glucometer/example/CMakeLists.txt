cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD yd_rp2040_4m)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(disp C CXX ASM)
pico_sdk_init()

add_executable(disp button.c lcd.c main.c)
target_link_libraries(disp pico_bootsel_via_double_reset pico_stdlib
  pico_status_led hardware_i2c)
pico_add_extra_outputs(disp)
