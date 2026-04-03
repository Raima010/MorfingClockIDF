# components/arduino_ssl_fix/CMakeLists.txt
# Force compilation of ssl_client.cpp for NetworkClientSecure

# Path to Arduino-ESP32 managed component
set(ARDUINO_PATH "${CMAKE_SOURCE_DIR}/managed_components/espressif__arduino-esp32")

idf_component_register(
    SRCS 
        "${ARDUINO_PATH}/libraries/NetworkClientSecure/src/ssl_client.cpp"
    INCLUDE_DIRS 
        "${ARDUINO_PATH}/libraries/NetworkClientSecure/src"
    REQUIRES 
        arduino-esp32
)

# Ensure correct macros so symbols are exported
target_compile_definitions(${COMPONENT_LIB} PRIVATE ARDUINO_ARCH_ESP32)