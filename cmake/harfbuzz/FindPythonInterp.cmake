# Replacement for the FindPythonInterp module removed by policy CMP0148.
# HarfBuzz includes it only to get PYTHON_EXECUTABLE.

find_package(Python3 COMPONENTS Interpreter QUIET)

if (Python3_Interpreter_FOUND)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    set(PYTHONINTERP_FOUND TRUE)
    set(PYTHON_VERSION_STRING "${Python3_VERSION}")
else()
    set(PYTHONINTERP_FOUND FALSE)
endif()
