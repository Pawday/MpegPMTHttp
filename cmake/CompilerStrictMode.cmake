function (target_set_strict_compilation TARGET)
    if (${CMAKE_C_COMPILER_ID} STREQUAL "GCC" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_compile_options(${TARGET} PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic")
    endif()

    if (${CMAKE_C_COMPILER_ID} STREQUAL "GCC")
         # see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
        target_compile_options(${TARGET} PRIVATE "-Wno-missing-braces")
    endif()

endfunction()

function (target_try_add_llvm_sanitizer TARGET)
    if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
	message(STATUS "Target ${TARGET} will be compiled with LLVM Asan")
        target_compile_options(${TARGET} PRIVATE "-fsanitize=address")
        target_compile_options(${TARGET} PRIVATE "-fno-optimize-sibling-calls")
        target_compile_options(${TARGET} PRIVATE "-fno-omit-frame-pointer")
    endif()
endfunction()

function (target_try_link_llvm_sanitizer TARGET)
    if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
	message(STATUS "Target ${TARGET} will be linked with LLVM Asan")
        target_link_options(${TARGET} PRIVATE "-fsanitize=address")
        target_link_options(${TARGET} PRIVATE "-fno-optimize-sibling-calls")
        target_link_options(${TARGET} PRIVATE "-fno-omit-frame-pointer")
    endif()
endfunction()
