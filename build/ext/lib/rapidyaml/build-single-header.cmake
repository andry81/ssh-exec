cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED RAPIDYAML_ROOT)
  message(FATAL_ERROR "RAPIDYAML_ROOT variable is not defined")
endif()
if(NOT EXISTS "${RAPIDYAML_ROOT}")
  message(FATAL_ERROR "RAPIDYAML_ROOT does not exists: `${RAPIDYAML_ROOT}`")
endif()

find_package(Python3 COMPONENTS Interpreter)

set(singleheader "${RAPIDYAML_ROOT}/src_singleheader/ryml_all.hpp")
set(amscript "${RAPIDYAML_ROOT}/tools/amalgamate.py")

execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${RAPIDYAML_ROOT}/tools/amalgamate.py" "${RAPIDYAML_ROOT}/src_singleheader/ryml_all.hpp")
