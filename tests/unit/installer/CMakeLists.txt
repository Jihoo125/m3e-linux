add_executable(matinstall_core_tests test_core.cpp)
target_link_libraries(matinstall_core_tests PRIVATE matinstall_core)
target_compile_features(matinstall_core_tests PRIVATE cxx_std_20)
set_target_properties(matinstall_core_tests PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
  target_compile_options(matinstall_core_tests PRIVATE /W4 /permissive- /utf-8)
  if(MATINSTALL_WERROR)
    target_compile_options(matinstall_core_tests PRIVATE /WX)
  endif()
else()
  target_compile_options(matinstall_core_tests PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wformat=2)
  if(MATINSTALL_WERROR)
    target_compile_options(matinstall_core_tests PRIVATE -Werror)
  endif()
endif()
add_test(NAME matinstall_core_tests COMMAND matinstall_core_tests)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME matinstall_core_cli_safety
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_cli.py"
            "$<TARGET_FILE:matinstall-core>"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../installer/matinstall-core/examples/answer.json")
endif()
