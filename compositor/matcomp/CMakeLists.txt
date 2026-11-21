cmake_minimum_required(VERSION 3.16)
project(MaterializedCompositor LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLR REQUIRED IMPORTED_TARGET wlroots wayland-server xkbcommon libinput pixman-1)
set(WL_PROTOCOLS "${CMAKE_CURRENT_SOURCE_DIR}/../protocols")
add_custom_command(OUTPUT wlr-layer-shell-unstable-v1-protocol.h
    COMMAND wayland-scanner server-header "${WL_PROTOCOLS}/wlr-layer-shell-unstable-v1.xml" wlr-layer-shell-unstable-v1-protocol.h
    DEPENDS "${WL_PROTOCOLS}/wlr-layer-shell-unstable-v1.xml")
add_custom_command(OUTPUT xdg-shell-protocol.h
    COMMAND wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
    DEPENDS /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)
add_custom_command(OUTPUT xdg-decoration-server-protocol.h
    COMMAND wayland-scanner server-header "${WL_PROTOCOLS}/xdg-decoration-unstable-v1.xml" xdg-decoration-server-protocol.h
    DEPENDS "${WL_PROTOCOLS}/xdg-decoration-unstable-v1.xml")
add_executable(matcomp src/main.cpp src/server.cpp src/backend.cpp src/window.cpp src/config.cpp
    wlr-layer-shell-unstable-v1-protocol.h xdg-decoration-server-protocol.h xdg-shell-protocol.h)
target_include_directories(matcomp PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(matcomp PRIVATE PkgConfig::WLR)
target_compile_definitions(matcomp PRIVATE WLR_USE_UNSTABLE)
