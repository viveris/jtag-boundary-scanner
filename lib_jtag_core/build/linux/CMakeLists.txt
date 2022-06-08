cmake_minimum_required (VERSION 2.8.11)

# Build shared library in place for legacy support
add_custom_target(
        libjtag_core ALL
	COMMAND make all
        COMMENT "Building shared library using legacy support"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
        libjtag_static
	COMMAND make libjtag_core.a
        COMMENT "Building static library using legacy support"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

