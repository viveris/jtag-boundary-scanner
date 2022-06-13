# Create static and shared libraries and specify the source files.
# Any number of sources could be listed here.
add_library(xcSwigHere_ STATIC xcSwigHere.cxx)
add_library(xcSwigHere SHARED  xcSwigHere.cxx)

# For static library, enable position independent code
set_property(TARGET xcSwigHere_ PROPERTY POSITION_INDEPENDENT_CODE ON)

# Follow these directions to minimize the number of shared libraries generated:
#     Link the static binding to the shared version of the project's API library
target_link_libraries (xcSwigHere_ LINK_PUBLIC epCoreJTAG)
#     Link the shared binding to the static version of the project's API library
target_link_libraries (xcSwigHere LINK_PUBLIC epCoreJTAG_)

# Make sure the compiler can find include files for our library
# when other libraries or executables link to fftTOO
target_include_directories(xcSwigHere_ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(xcSwigHere PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
