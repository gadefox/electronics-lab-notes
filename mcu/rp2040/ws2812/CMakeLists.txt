cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pio_ws2812 C CXX ASM)
pico_sdk_init()

add_executable(pio_ws2812 ws2812.c)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(pio_ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_link_libraries(pio_ws2812 pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_ws2812)
