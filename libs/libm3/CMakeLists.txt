cmake_minimum_required(VERSION 3.22)
project(MaterializedColor VERSION 0.2.1 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(m3 STATIC
  src/color_utils.cpp
  src/contrast.cpp
  src/cam16.cpp
  src/hct.cpp
  src/palettes.cpp
  src/temperature.cpp
  src/dynamic_scheme.cpp
  src/quantize.cpp
  src/score.cpp
)
add_executable(m3-dump m3_dump.cpp)
target_link_libraries(m3-dump PRIVATE m3)


target_include_directories(m3
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE src
)
target_compile_features(m3 PUBLIC cxx_std_20)
target_compile_options(m3 PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
