cmake_minimum_required(VERSION 3.10)

project(MpegTsViaHTTP)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CompilerStrictMode)

add_subdirectory(mpegts)

find_package(CURL REQUIRED)
find_library(JSONC_LIB json-c REQUIRED)

add_executable(MpegTsHttp)

strictify_target_compilation(MpegTsHttp)

target_compile_features(MpegTsHttp PRIVATE c_std_11)
target_compile_definitions(MpegTsHttp PRIVATE CURL_STRICTER)


target_compile_options(MpegTsHttp PRIVATE "-fsanitize=address")
target_link_options(MpegTsHttp PRIVATE "-fsanitize=address")


target_compile_options(MpegTsHttp PRIVATE "-fno-optimize-sibling-calls")
target_link_options(MpegTsHttp PRIVATE "-fno-optimize-sibling-calls")

target_compile_options(MpegTsHttp PRIVATE "-fno-omit-frame-pointer")
target_link_options(MpegTsHttp PRIVATE "-fno-omit-frame-pointer")

target_sources(MpegTsHttp PRIVATE
	main.c
	media_source_proxy.c
        curl_media_source.c
        player.c
        pmt_output.c
        udp_media_source.c
	)

target_link_libraries(MpegTsHttp PRIVATE MpegTS)
target_link_libraries(MpegTsHttp PRIVATE CURL::libcurl)
target_link_libraries(MpegTsHttp PRIVATE ${JSONC_LIB})
