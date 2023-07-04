cmake_minimum_required(VERSION 3.10)

project(MpegTsViaHTTP)

add_subdirectory(mpegts)

find_package(CURL REQUIRED)


add_executable(MpegTsHttp main.c)
target_link_libraries(MpegTsHttp PRIVATE MpegTS)
target_link_libraries(MpegTsHttp PRIVATE CURL::libcurl)
