cmake_minimum_required(VERSION 3.16)
project(MaterializedScreenshot LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
set(PROTOCOL_XML "${CMAKE_CURRENT_SOURCE_DIR}/../../compositor/protocols/wlr-screencopy-unstable-v1.xml")
add_custom_command(OUTPUT wlr-screencopy-unstable-v1-client-protocol.h
    COMMAND wayland-scanner client-header "${PROTOCOL_XML}" wlr-screencopy-unstable-v1-client-protocol.h
    DEPENDS "${PROTOCOL_XML}")
add_custom_command(OUTPUT wlr-screencopy-unstable-v1-protocol.c
    COMMAND wayland-scanner private-code "${PROTOCOL_XML}" wlr-screencopy-unstable-v1-protocol.c
    DEPENDS "${PROTOCOL_XML}")
add_executable(matshot main.cpp screenshot_backend.cpp screenshot_backend.hpp
    wlr-screencopy-unstable-v1-client-protocol.h wlr-screencopy-unstable-v1-protocol.c)
target_include_directories(matshot PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
qt_add_resources(matshot resources PREFIX /matshot FILES Main.qml)
target_link_libraries(matshot PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick PkgConfig::WAYLAND_CLIENT)
