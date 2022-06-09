#load the cmake SWIG package
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
    
#load the package that SWIG uses to generate Python
find_package(PythonInterp 3 EXACT REQUIRED)
find_package(PythonLibs 3 EXACT REQUIRED)


#point to python headers
include_directories(${PYTHON_INCLUDE_DIRS})

# Need to be explicit about the include directoried used in SWIG
include_directories(${PROJECT_SOURCE_DIR}/test/imports)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# specify the SWIG interface for the module
set_property(SOURCE py3SwigHere.i PROPERTY CPLUSPLUS ON)
set_property(SOURCE py3SwigHere.i PROPERTY SWIG_MODULE_NAME pySwigHere)
#set_property(SOURCE py3SwigHere.i PROPERTY SWIG_MODULE_NAME py3SwigHere)

swig_add_library(
    pySwigHere
    # ALIAS py3SwigHere
    TYPE SHARED
    LANGUAGE python
    SOURCES py3SwigHere.i py3SwigHere.cxx)

SWIG_LINK_LIBRARIES(pySwigHere ${PYTHON_LIBRARIES} epCoreJTAG_ jtag_bsdl)

set_property(SOURCE ${swig_generated_file_fullname} PROPERTY OUTPUT_NAME pySwigHere)
get_property(SHARED_LIB_NAME SOURCE ${swig_generated_file_fullname} PROPERTY OUTPUT_NAME)

configure_file(init.in __init__.py @ONLY)
ADD_CUSTOM_TARGET(initPy3 ALL SOURCES __init__.py)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "__init__.py" )
