# Include bindings to projects API libraries
add_subdirectory(bindings)

# Demo Python3 file showing how to load and run the application
configure_file(py3UI.py py3UI.py @ONLY)
ADD_CUSTOM_TARGET(py3UI SOURCES py3UI.py)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "py3UI.py" )
