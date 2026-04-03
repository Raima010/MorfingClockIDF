# Use ${COMPONENT_DIR} to ensure we are looking in the right place
idf_component_register(SRC_DIRS "${COMPONENT_DIR}/src"
                       INCLUDE_DIRS "${COMPONENT_DIR}/src"
                       REQUIRES arduino-esp32 arduinojson)

