cmake_minimum_required(VERSION 3.26.0)
project(imgui LANGUAGES C CXX)

add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)

add_executable(imgui_example imgui/examples/example_glfw_vulkan/main.cpp)
target_link_libraries(imgui_example imgui glfw vulkan)

