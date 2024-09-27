cmake_minimum_required(VERSION 3.26)
project(the-vulkan-cookbook CXX)

find_package(VulkanLoader CONFIG REQUIRED)
find_package(vulkan-validationlayers CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

include(cmake/common.cmake)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

add_subdirectory(ch01)
