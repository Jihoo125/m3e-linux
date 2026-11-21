# MatVersion.cmake — single source of truth for the version string.
#
# The canonical version lives in build/version.txt so that shell scripts (ISO
# build, packaging, release tooling) and CMake agree without duplication.

function(mat_read_version out_string out_major out_minor out_patch)
  set(_file "${CMAKE_SOURCE_DIR}/build/version.txt")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "MatVersion: ${_file} not found")
  endif()
  file(READ "${_file}" _raw)
  string(STRIP "${_raw}" _v)
  if(NOT _v MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    message(FATAL_ERROR "MatVersion: '${_v}' is not MAJOR.MINOR.PATCH")
  endif()
  set(${out_string} "${_v}" PARENT_SCOPE)
  set(${out_major}  "${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(${out_minor}  "${CMAKE_MATCH_2}" PARENT_SCOPE)
  set(${out_patch}  "${CMAKE_MATCH_3}" PARENT_SCOPE)
endfunction()
