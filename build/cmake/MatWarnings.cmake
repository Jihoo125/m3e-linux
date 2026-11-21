# MatWarnings.cmake — warning policy for the whole tree.
#
# Warnings are errors in CI (-DMAT_WERROR=ON) and advisory locally. libm3 is
# dependency-free so it gets the strictest numeric warnings; GUI code is a notch
# looser because Qt headers produce noise under -Wextra.

option(MAT_WERROR "Treat compiler warnings as errors" OFF)

function(mat_set_warnings)
  if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
    if(MAT_WERROR)
      add_compile_options(/WX)
    endif()
  else()
    add_compile_options(-Wall -Wextra -Wpedantic
                        -Wshadow -Wconversion -Wsign-conversion
                        -Wnon-virtual-dtor -Woverloaded-virtual
                        -Wcast-qual -Wformat=2)
    if(MAT_WERROR)
      add_compile_options(-Werror)
    endif()
  endif()
endfunction()

# Apply the relaxed Qt-friendly policy to a single target.
function(mat_relax_warnings target)
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE -Wno-conversion -Wno-sign-conversion)
  endif()
endfunction()
