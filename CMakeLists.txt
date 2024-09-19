cmake_minimum_required(VERSION 3.26)
project(the-vulkan-cookbook CXX)

find_package(VulkanLoader CONFIG REQUIRED)
find_package(vulkan-validationlayers CONFIG REQUIRED)

include(cmake/common.cmake)

add_subdirectory(ch01)
