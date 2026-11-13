cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD yd_rp2040_4m)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(remote C CXX ASM)
pico_sdk_init()

add_executable(remote main.c)
target_link_libraries(remote pico_bootsel_via_double_reset pico_stdlib)
pico_enable_stdio_usb(remote 1)
pico_enable_stdio_uart(remote 0)
pico_add_extra_outputs(remote)
