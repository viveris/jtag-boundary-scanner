# Create a library called "xcSwigHere" and specify the source files.
# The extension is already found. Any number of sources could be listed here.
add_subdirectory(bindings)

# CPP console application
add_executable (cppUI cppUI.cxx)
set_property(TARGET cppUI PROPERTY POSITION_INDEPENDENT_CODE ON)
#set_target_properties(cppUI PROPERTIES LINK_LIBRARIES -pthread)

# Link the executable to the libraries upon which it depends. 
# Libraries with public include directories will have those link 
# directories automatically configured when building the executable
target_link_libraries (cppUI PUBLIC xcSwigHere epCoreJTAG_ jtag_bsdl)
