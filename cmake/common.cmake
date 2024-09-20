cmake_minimum_required(VERSION 3.15)
project(the-vulkan-cookbook CXX)

function (apply_vulkan_common_properties)
  set(options "")
  set(oneValueArgs TARGET)
  set(multiValueArgs "")
  cmake_parse_arguments(APPLY_VULKAN_COMMON_PROPERTIES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  target_compile_definitions(${APPLY_VULKAN_COMMON_PROPERTIES_TARGET}
      PUBLIC
        $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
  )

  target_link_libraries(${APPLY_VULKAN_COMMON_PROPERTIES_TARGET}
      PRIVATE
        Vulkan::Loader
        vulkan-validationlayers::vulkan-validationlayers
  )
endfunction()

function (add_vulkan_executable)
  set(options "")
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(ADD_VULKAN_EXECUTABLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_executable(${ADD_VULKAN_EXECUTABLE_TARGET})
  target_sources(${ADD_VULKAN_EXECUTABLE_TARGET}
      PRIVATE ${ADD_VULKAN_EXECUTABLE_SOURCES}
  )

  target_compile_definitions(${ADD_VULKAN_EXECUTABLE_TARGET}
      PUBLIC
      # Vulkan
      $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
      # glfw
      $<$<PLATFORM_ID:Windows>:GLFW_EXPOSE_NATIVE_WIN32 GLFW_EXPOSE_NATIVE_WGL>
  )

  target_link_libraries(${ADD_VULKAN_EXECUTABLE_TARGET}
      PRIVATE
      Vulkan::Loader
      vulkan-validationlayers::vulkan-validationlayers
      glfw
  )
endfunction()
