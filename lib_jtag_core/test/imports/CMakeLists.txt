# Create static and shared libraries and specify the source files.
# Any number of sources could be listed here.

add_library(epCoreJTAG_ STATIC epCoreJTAG.cxx)
add_library(epCoreJTAG SHARED epCoreJTAG.cxx)

# For static library, enable position independent code
set_property(TARGET epCoreJTAG_ PROPERTY POSITION_INDEPENDENT_CODE ON)

# Make sure the compiler can find include files for our library
# when other libraries or executables link to fftTOO
target_include_directories (epCoreJTAG_ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${STREETVIEWJTAG_SOURCE_DIR}/src")
set_property(TARGET epCoreJTAG_ PROPERTY INTERFACE_LINK_LIBRARIES jtag_bsdl)
target_link_libraries(epCoreJTAG_ jtag_bsdl)

target_include_directories (epCoreJTAG PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${STREETVIEWJTAG_SOURCE_DIR}/src")
set_property(TARGET epCoreJTAG PROPERTY INTERFACE_LINK_LIBRARIES jtag_bsdl)
target_link_libraries(epCoreJTAG jtag_bsdl)

add_executable(uiCoreJTAG uiCoreJTAG.cxx)
target_link_libraries(uiCoreJTAG epCoreJTAG jtag_bsdl)

add_executable(uiCoreJTAG_ uiCoreJTAG.cxx)
target_link_libraries(uiCoreJTAG_ epCoreJTAG jtag_bsdl)
