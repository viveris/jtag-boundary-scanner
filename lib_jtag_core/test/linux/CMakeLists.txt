cmake_minimum_required (VERSION 2.8.11)

# Build shared library in place for legacy support
add_custom_target(BSLDtoFloorplan ALL
                  COMMENT "Building app using legacy support"
                  COMMAND make all
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

