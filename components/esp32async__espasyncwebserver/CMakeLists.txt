set(COMPONENT_SRCDIRS "src")
set(COMPONENT_ADD_INCLUDEDIRS "src")

# ADD THIS LINE: This tells the webserver it needs the Arduino framework
set(COMPONENT_REQUIRES arduino-esp32 arduinojson)

register_component()