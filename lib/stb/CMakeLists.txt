cmake_minimum_required(VERSION 3.26.0)
project(stb LANGUAGES C)

file(GLOB STB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/stb/*.c")

add_library(stb ${STB_SOURCES})

target_include_directories(stb SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stb)
target_compile_definitions(stb PUBLIC)

target_compile_options(stb PRIVATE
)

