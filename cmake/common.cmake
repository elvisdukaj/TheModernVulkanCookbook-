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
