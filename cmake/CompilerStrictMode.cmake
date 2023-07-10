function (strictify_target_compilation TARGET)
    if (${CMAKE_C_COMPILER_ID} STREQUAL "GCC" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_compile_options(${TARGET} PRIVATE "-Wall" "-Wextra" "-Werror" "-pedantic")
    endif()

    if (${CMAKE_C_COMPILER_ID} STREQUAL "GCC")
         # see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
        target_compile_options(${TARGET} PRIVATE "-Wno-missing-braces")
    endif()

endfunction()
