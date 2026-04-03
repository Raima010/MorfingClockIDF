# 1. Find EVERY .cpp file in all subdirectories (Packets, Out, Internals, etc.)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 2. Find EVERY directory that contains a header file (.h or .hpp)
# This is critical so the compiler can find "OutPacket.hpp" inside "Packets/Out"
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")
set(INCLUDE_DIRS "")
foreach(_header_file ${HEADERS})
    get_filename_component(_dir ${_header_file} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

# 3. Register the component with all found sources and all include paths
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${INCLUDE_DIRS}
                       REQUIRES arduino-esp32 asynctcp)

# 4. Mandatory: Disable RTTI for AsyncMQTT compatibility
target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti)

