cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pgen C CXX ASM)
pico_sdk_init()

add_executable(pgen make.c)

target_link_libraries(pgen pico_stdlib hardware_pwm)
pico_add_extra_outputs(pgen)
