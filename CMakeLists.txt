cmake_minimum_required(VERSION 3.11)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles/*." )
endif()

project(ncmdump CXX)

set(CMAKE_CXX_STANDARD 11)

include(ucm.cmake)
ucm_set_runtime(STATIC)

add_executable(ncmdump main.cpp cJSON.cpp cJSON.h)

find_package(OpenSSL REQUIRED)
target_link_libraries(ncmdump PRIVATE OpenSSL::Crypto)

find_library(TagLib tag)
if(${TagLib-NOTFOUND})
    message(FATAL_ERROR "cannot find TagLib")
endif()

target_link_libraries(ncmdump PRIVATE "${TagLib}")

find_package(ZLIB REQUIRED)
target_link_libraries(ncmdump PRIVATE ZLIB::ZLIB)
