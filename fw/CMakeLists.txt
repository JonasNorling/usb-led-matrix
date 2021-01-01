list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)
set(BOARD usb_led_matrix)

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usb-led-matrix)

target_sources(app PRIVATE src/main.c)
