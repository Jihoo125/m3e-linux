cmake_minimum_required(VERSION 3.16)
if(NOT COMMAND qt_add_qml_module)
    message(FATAL_ERROR "Build the Materialized QML module before the gallery")
endif()

set(_gallery_bin "${CMAKE_BINARY_DIR}/bin")
if(QT_VERSION VERSION_LESS "6.3.0")
    # Qt 6.2 has qt_add_qml_module but no qt_add_executable.
    add_executable(matgallery main.cpp)
    target_compile_definitions(matgallery PRIVATE
        MAT_QML_OUTPUT_DIRECTORY="${_gallery_bin}"
        MAT_QML_LIBRARY_DIRECTORY="${CMAKE_BINARY_DIR}/lib")
    qt_add_qml_module(matgallery URI Materialized.Gallery VERSION 1.0
        QML_FILES Main.qml ColorSwatch.qml RESOURCE_PREFIX /)
    target_link_libraries(matgallery PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick m3_qt)
else()
    qt_add_executable(matgallery main.cpp)
    target_compile_definitions(matgallery PRIVATE
        MAT_QML_OUTPUT_DIRECTORY="${_gallery_bin}"
        MAT_QML_LIBRARY_DIRECTORY="${CMAKE_BINARY_DIR}/lib")
    qt_add_qml_module(matgallery URI Materialized.Gallery VERSION 1.0
        QML_FILES Main.qml ColorSwatch.qml RESOURCE_PREFIX /)
    target_link_libraries(matgallery PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick m3_qt)
endif()
set_target_properties(matgallery PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${_gallery_bin}")
install(TARGETS matgallery RUNTIME DESTINATION bin)
