cmake_minimum_required(VERSION 3.10)

project(PMTPlayer)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CompilerStrictMode)

add_subdirectory(mpegts)

find_package(CURL REQUIRED)
find_library(JSONC_LIB json-c REQUIRED)

add_executable(PMTPlayer)

target_set_strict_compilation(PMTPlayer)

target_compile_features(PMTPlayer PRIVATE c_std_11)
target_compile_definitions(PMTPlayer PRIVATE CURL_STRICTER)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_add_llvm_sanitizer(PMTPlayer)
    target_link_llvm_sanitizer(PMTPlayer)
endif()



target_sources(PMTPlayer PRIVATE
    main.c
    media_source_proxy.c
    curl_media_source.c
    player.c
    pmt_output.c
    udp_media_source.c
    )

target_link_libraries(PMTPlayer PRIVATE MpegTS)
target_link_libraries(PMTPlayer PRIVATE CURL::libcurl)
target_link_libraries(PMTPlayer PRIVATE ${JSONC_LIB})
