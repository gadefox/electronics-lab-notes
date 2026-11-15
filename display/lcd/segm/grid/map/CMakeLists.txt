cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD yd_rp2040_4m)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(comseg C CXX ASM)
pico_sdk_init()

add_executable(comseg main.c)
target_link_libraries(comseg pico_bootsel_via_double_reset pico_stdlib)
pico_enable_stdio_usb(comseg 1)
pico_enable_stdio_uart(comseg 0)
pico_add_extra_outputs(comseg)
