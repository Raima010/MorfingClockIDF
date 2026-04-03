idf_component_register(SRCS "wifi_mqtt.cpp" 
                            "main.cpp"
                            "weather.cpp" 
                            "clock.cpp" 
                            "digit.cpp" 
                            "Mem.cpp"
                            "web_ota.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES mbedtls arduino-esp32 espasyncwebserver asynctcp AsyncMQTT_Generic ESP-WiFiSettings SmartMatrix arduinojson)
