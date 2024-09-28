#include <algorithm>
#include <functional>
#include <iterator>
#include <print>
#include <ranges>
#include <vector>

#include <vulkan/vulkan.h>

namespace ranges = std::ranges;
namespace views = std::ranges::views;

#define VK_CALL(vFun)                                                                                                  \
  {                                                                                                                    \
    const auto res = vFun;                                                                                             \
    if (res != VK_SUCCESS) {                                                                                           \
      std::println(#vFun " failed with error {}", static_cast<int>(res));                                              \
      std::exit(res);                                                                                                  \
    }                                                                                                                  \
  }

std::vector<VkLayerProperties> enumerateInstanceLayerProperties() {
  uint32_t layersCount{0};
  vkEnumerateInstanceLayerProperties(&layersCount, nullptr);

  std::vector<VkLayerProperties> layersProperties(layersCount);
  vkEnumerateInstanceLayerProperties(&layersCount, layersProperties.data());

  return layersProperties;
}

std::vector<std::string> getAvailableInstanceLayersName() {
  auto layerProperties = enumerateInstanceLayerProperties();

  const auto extractNameFromProperties = [](const VkLayerProperties& properties) -> std::string {
    return std::string{properties.layerName};
  };

  auto availableInstanceLayersName =
      views::transform(layerProperties, extractNameFromProperties) | ranges::to<std::vector<std::string>>();

  return availableInstanceLayersName;
}

std::vector<VkExtensionProperties> enumerateExtensionsProperties() {
  uint32_t extensionsCount{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

  std::vector<VkExtensionProperties> extensionsProperties(extensionsCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionsProperties.data());

  return extensionsProperties;
}

std::vector<std::string> getLayerExtensionsName() {
  auto layerProperties = enumerateExtensionsProperties();

  auto extractExtensionName = [](const VkExtensionProperties& properties) -> std::string {
    return std::string{properties.extensionName};
  };

  return views::transform(layerProperties, extractExtensionName) | ranges::to<std::vector<std::string>>();
}

void print(std::ranges::input_range auto&& layers) {
  ranges::for_each(layers, [](const auto& layer) { std::println(" - {}", layer); });
}

void printVulkanInstanceAndExtensions(std::ranges::input_range auto&& availableLayers,
                                      std::ranges::input_range auto&& availableExtensions,
                                      std::ranges::input_range auto&& enabledLayers,
                                      std::ranges::input_range auto&& enabledExtension) {
  std::println("\n*****************");
  std::println("Available Layers:");
  print(availableLayers);

  std::println("Available Extensions:");
  print(availableExtensions);

  std::println("\n*****************");
  std::println("Enabled layers:");
  print(enabledLayers);

  std::println("Enabled Extensions:");
  print(enabledExtension);
}

int main() {
  const std::vector<std::string> requestedInstanceLayers = {"VK_LAYER_KHRONOS_validation"};

  const std::vector<std::string> requestedInstanceExtensions = {
#if defined(VK_KHR_win32_surface)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_METAL_EXT)
      VK_EXT_METAL_SURFACE_EXTENSION_NAME, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
#if defined(VK_EXT_debug_utils)
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#if defined(VK_KHR_surface)
      VK_KHR_SURFACE_EXTENSION_NAME,
#endif
  };

  const auto isLayerRequired = [&requestedInstanceLayers](const std::string& name) {
    auto it = ranges::find(requestedInstanceLayers, name);
    const auto isIncluded = it != std::end(requestedInstanceLayers);
    return isIncluded;
  };
  const auto isExtensionRequired = [&requestedInstanceExtensions](const std::string& name) {
    auto it = ranges::find(requestedInstanceExtensions, name);
    const auto isIncluded = it != std::end(requestedInstanceExtensions);
    return isIncluded;
  };

  const auto availableLayers = getAvailableInstanceLayersName();
  const auto availableExtensions = getLayerExtensionsName();

  const auto enabledInstanceLayers = availableLayers | views::filter(isLayerRequired) |
                                     views::transform(std::mem_fn(&std::string::c_str)) |
                                     ranges::to<std::vector<const char*>>();

  const auto enabledInstanceExtensions = availableExtensions | views::filter(isExtensionRequired) |
                                         views::transform(std::mem_fn(&std::string::c_str)) |
                                         ranges::to<std::vector<const char*>>();

  printVulkanInstanceAndExtensions(availableLayers, availableExtensions, enabledInstanceLayers,
                                   enabledInstanceExtensions);

  const VkApplicationInfo applicationInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                          .pApplicationName = "ch01",
                                          .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .apiVersion = VK_API_VERSION_1_3};

  const VkInstanceCreateInfo instanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(VK_USE_PLATFORM_METAL_EXT)
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size()),
      .ppEnabledLayerNames = enabledInstanceLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size()),
      .ppEnabledExtensionNames = enabledInstanceExtensions.data()};

  VkInstance vulkanInstance{VK_NULL_HANDLE};
  VK_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance));

  vkDestroyInstance(vulkanInstance, nullptr);

  return EXIT_SUCCESS;
}
