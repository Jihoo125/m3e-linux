cmake_minimum_required(VERSION 3.16)
project(MaterializedQml LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

if(QT_VERSION VERSION_LESS "6.5.0")
    # Qt 6.2/6.4 cannot load optional plugins from resource-only qmldirs. Use
    # explicit file imports for the development compatibility layer. Release
    # builds use qt_add_qml_module + qmlcachegen (Debian 13 / Qt 6.8).
    file(GLOB M3_QML_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/qml/M3*.qml")
    set(MAT_GENERATED "${CMAKE_BINARY_DIR}/qml/Materialized")
    set(MAT_QMLDIR_CONTENT "module Materialized\nplugin m3plugin\nclassname MaterializedPlugin\n")
    foreach(qml_file IN LISTS M3_QML_FILES)
        get_filename_component(qml_name "${qml_file}" NAME_WE)
        set(destination "${MAT_GENERATED}/${qml_name}.qml")
        add_custom_command(OUTPUT "${destination}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${MAT_GENERATED}"
            COMMAND "${CMAKE_COMMAND}" -E copy "${qml_file}" "${destination}"
            DEPENDS "${qml_file}"
        )
        list(APPEND MAT_COPIED_QML "${destination}")
        string(APPEND MAT_QMLDIR_CONTENT "${qml_name} 1.0 ${qml_name}.qml\n")
    endforeach()
    file(GENERATE OUTPUT "${MAT_GENERATED}/qmldir" CONTENT "${MAT_QMLDIR_CONTENT}")
endif()

add_library(m3_qt STATIC m3theme.cpp m3theme.hpp)
set_target_properties(m3_qt PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(m3_qt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(m3_qt PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
set_target_properties(m3_qt PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(m3plugin MODULE m3plugin.cpp)
target_link_libraries(m3plugin PRIVATE m3_qt Qt6::Qml)
set_target_properties(m3plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/Materialized"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/Materialized"
    PREFIX "lib"
    OUTPUT_NAME "m3plugin"
)

add_custom_target(m3_qml_files ALL DEPENDS ${MAT_COPIED_QML})

if(MAT_ENABLE_TESTS)
    enable_testing()
    add_executable(m3theme-tests theme_test.cpp)
    target_link_libraries(m3theme-tests PRIVATE m3_qt Qt6::Qml Qt6::Gui)
    add_test(NAME m3theme COMMAND m3theme-tests)
endif()

add_subdirectory(../../apps/matgallery ${CMAKE_BINARY_DIR}/gallery)
