cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ws2812 C CXX ASM)
pico_sdk_init()

add_executable(ws2812 ws2812.c)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})
pico_generate_pio_header(ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${GENERATED_DIR})

target_link_libraries(ws2812 pico_stdlib hardware_pio)
pico_add_extra_outputs(ws2812)
