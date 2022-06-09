# Include language-specific user-interfaces
add_subdirectory(cxx)
find_package(PythonInterp 3 EXACT QUIET)
if(${PYTHONINTERP_FOUND})
    find_package(PythonLibs 3 EXACT QUIET)
    if(${PYTHONLIBS_FOUND})
        add_subdirectory(py3)
    endif()
endif()

