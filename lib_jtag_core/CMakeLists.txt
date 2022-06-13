# CMakeLists files in this project can
# refer to the root source directory of the project as ${STREETVIEWJTAG_SOURCE_DIR} and
# to the root binary directory of the project as ${STREETVIEWJTAG_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (STREETVIEWJTAG)

# Recurse into the "build", "src" and "test" subdirectories. 
add_subdirectory (build)
add_subdirectory (src)
add_subdirectory (test)

