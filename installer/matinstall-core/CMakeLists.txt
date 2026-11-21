cmake_minimum_required(VERSION 3.22)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(matinstall-core VERSION 0.2.0 LANGUAGES CXX)
endif()

option(MATINSTALL_BUILD_TESTS "Build dependency-free matinstall-core unit tests" ${PROJECT_IS_TOP_LEVEL})
option(MATINSTALL_WERROR "Treat matinstall-core compiler warnings as errors" OFF)

add_library(matinstall_core STATIC
  src/answer.cpp
  src/installer.cpp
  src/json.cpp
  src/planner.cpp
  src/process.cpp
)
add_library(Materialized::matinstall_core ALIAS matinstall_core)

target_compile_features(matinstall_core PUBLIC cxx_std_20)
target_include_directories(matinstall_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
set_target_properties(matinstall_core PROPERTIES
  CXX_EXTENSIONS OFF
  OUTPUT_NAME matinstall_core
)

add_executable(matinstall-core src/main.cpp)
target_link_libraries(matinstall-core PRIVATE matinstall_core)
target_compile_features(matinstall-core PRIVATE cxx_std_20)
set_target_properties(matinstall-core PROPERTIES CXX_EXTENSIONS OFF)

foreach(target IN ITEMS matinstall_core matinstall-core)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    if(MATINSTALL_WERROR)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
      -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-qual -Wformat=2
    )
    if(MATINSTALL_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endforeach()

if(MATINSTALL_BUILD_TESTS)
  include(CTest)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  set(MATINSTALL_TEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/unit/installer")
  if(EXISTS "${MATINSTALL_TEST_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${MATINSTALL_TEST_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/matinstall-tests")
  else()
    message(FATAL_ERROR "MATINSTALL_BUILD_TESTS=ON requires tests/unit/installer")
  endif()
endif()
