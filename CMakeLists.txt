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

target_sources(MpegTsHttp PRIVATE
	main.c
	curl_player.c
	)

target_link_libraries(MpegTsHttp PRIVATE MpegTS)
target_link_libraries(MpegTsHttp PRIVATE CURL::libcurl)
target_link_libraries(MpegTsHttp PRIVATE ${JSONC_LIB})
