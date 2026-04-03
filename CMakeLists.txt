set(ENV{ARDUINO_SKIP_IDF_VERSION_CHECK} 1)

cmake_minimum_required(VERSION 3.16)

# Add this line to tell ESP-IDF to look in your local components folder

add_compile_options("-w")

add_compile_definitions(GPIOPINOUT=ESP32_FORUM_PINOUT)

set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(morfclk)

idf_build_set_property(COMPILE_OPTIONS "-DARDUINO=100" APPEND)
