cmake_minimum_required(VERSION 3.22)

project(MaterializedLinux
  VERSION 0.2.0
  DESCRIPTION "Materialized Linux — a Material 3 desktop distribution"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MAT_ENABLE_COMPOSITOR "Build the wlroots-based compositor" OFF)
option(MAT_ENABLE_XWAYLAND "Enable XWayland in the compositor" OFF)
option(MAT_ENABLE_APPS "Build the supported Alpha 0.1 applications" OFF)
option(MAT_ENABLE_SHELL "Build matsessiond and matshell" OFF)
option(MAT_ENABLE_INSTALLER "Build the safe installer engine" OFF)
option(MAT_ENABLE_TESTS "Build component unit tests" ON)
option(MAT_ENABLE_TOOLS "Build development tools" OFF)
option(MAT_QMLCACHEGEN "Pre-compile QML where supported" OFF)
set(_gui_default OFF)
if(MAT_ENABLE_APPS OR MAT_ENABLE_SHELL OR MAT_ENABLE_INSTALLER)
  set(_gui_default ON)
endif()
option(MAT_BUILD_GUI "Build Qt-dependent components" ${_gui_default})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
include(MatWarnings)
include(MatVersion)
mat_set_warnings()
mat_read_version(MAT_VERSION_STRING MAT_VERSION_MAJOR MAT_VERSION_MINOR MAT_VERSION_PATCH)

add_subdirectory(libs/libm3)

function(mat_add_component directory)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/CMakeLists.txt")
    add_subdirectory("${directory}")
  else()
    message(STATUS "Skipping ${directory}: not included in the Alpha 0.1 tree")
  endif()
endfunction()

if(MAT_ENABLE_TOOLS)
  mat_add_component(tools)
endif()

if(MAT_BUILD_GUI)
  # Development also supports Ubuntu 22.04's Qt 6.2; the distribution target is
  # Qt 6.8 on Debian 13. Do not require the newer development host version.
  find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Qml Quick DBus)
  add_subdirectory(libs/libmatdbus)
  mat_add_component(libs/mattoolkit)
  add_subdirectory(libs/libm3-qt)

  if(MAT_ENABLE_SHELL)
    add_subdirectory(shell/matsessiond)
    add_subdirectory(shell/matshell)
    # Lock/wallpaper are later milestones, not silent placeholder targets.
    mat_add_component(shell/matwallpaper)
  endif()
  if(MAT_ENABLE_APPS)
    add_subdirectory(apps)
  endif()
  if(MAT_ENABLE_INSTALLER)
    add_subdirectory(installer/matinstall-core)
    mat_add_component(installer/matinstall)
  endif()
endif()

if(MAT_ENABLE_COMPOSITOR)
  add_subdirectory(compositor/matcomp)
endif()

if(MAT_ENABLE_TESTS)
  enable_testing()
  mat_add_component(tests)
endif()

message(STATUS "Materialized Linux ${MAT_VERSION_STRING}")
message(STATUS "  GUI: ${MAT_BUILD_GUI}; shell: ${MAT_ENABLE_SHELL}; apps: ${MAT_ENABLE_APPS}")
message(STATUS "  compositor: ${MAT_ENABLE_COMPOSITOR}; installer: ${MAT_ENABLE_INSTALLER}")
