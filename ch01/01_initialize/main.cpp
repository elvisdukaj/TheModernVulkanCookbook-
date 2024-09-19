#include <print>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include <vulkan/vulkan.h>

#define VK_CALL(vFun)                                                                   \
    {                                                                                   \
        const auto res = vFun;                                                          \
        if (res != VK_SUCCESS) {                                                        \
            std::println(#vFun " failed with error {}", static_cast<uint32_t>(res));    \
            std::exit(res);                                                             \
        }                                                                               \
    }

std::vector<VkLayerProperties> enumerateLayerProperties() {
    uint32_t layersCount{0};
    vkEnumerateInstanceLayerProperties(&layersCount, nullptr);

    std::vector<VkLayerProperties> layersProperties(layersCount);
    vkEnumerateInstanceLayerProperties(&layersCount, layersProperties.data());

    return layersProperties;
}

std::vector<std::string> getAvailableLayersName() {
    auto layerProperties = enumerateLayerProperties();
    std::vector<std::string> availableLayers(layerProperties.size());

    std::transform(begin(layerProperties), end(layerProperties), begin(availableLayers),
                   [](const VkLayerProperties &properties) -> std::string {
                       return std::string{properties.layerName};
                   });

    return availableLayers;
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

    std::vector<std::string> layerExtensions(layerProperties.size());
    std::transform(begin(layerProperties), end(layerProperties), begin(layerExtensions),
                   [](const VkExtensionProperties &properties) -> std::string {
                       return std::string{properties.extensionName};
                   });

    return layerExtensions;
}


std::unordered_set<std::string> filterExtensions(std::vector<std::string> availableExtensions,
                                                 std::vector<std::string> requestedExtensions) {
    std::sort(availableExtensions.begin(), availableExtensions.end());
    std::sort(requestedExtensions.begin(), requestedExtensions.end());
    std::vector<std::string> result;

    std::set_intersection(
        availableExtensions.begin(),
        availableExtensions.end(),
        requestedExtensions.begin(),
        requestedExtensions.end(),
        std::back_inserter(result));

    return std::unordered_set<std::string>{
        result.begin(), result.end()
    };
}

int main() {

    const std::vector<std::string> requestedInstanceLayers = {"VK_LAYER_KHRONOS_validation"};

    const std::vector<std::string> requestedInstanceExtensions = {
#if defined(VK_KHR_win32_surface)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_EXT_debug_utils)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#if defined(VK_KHR_surface)
        VK_KHR_SURFACE_EXTENSION_NAME,
#endif
    };

    const auto availableLayers = getAvailableLayersName();
    const auto availableExtensions = getLayerExtensionsName();

    const auto enabledInstanceLayers = filterExtensions(availableLayers, requestedInstanceLayers);
    const auto enabledInstanceExtensions = filterExtensions(availableExtensions, requestedInstanceExtensions);

    std::vector<const char *> viewEnabledInstanceLayers(enabledInstanceLayers.size());
    std::transform(begin(enabledInstanceLayers), end(enabledInstanceLayers), begin(viewEnabledInstanceLayers),
                   [](const std::string &name) -> const char * {
                       return name.c_str();
                   });
    std::vector<const char *> viewEnabledExtensions(enabledInstanceExtensions.size());
    std::transform(begin(enabledInstanceExtensions), end(enabledInstanceExtensions), begin(viewEnabledExtensions),
                   [](const std::string &name) -> const char * {
                       return name.c_str();
                   });

    auto extensionsProperties = enumerateLayerProperties();

    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "ch01",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    const VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(viewEnabledInstanceLayers.size()),
        .ppEnabledLayerNames = viewEnabledInstanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(viewEnabledExtensions.size()),
        .ppEnabledExtensionNames = viewEnabledExtensions.data()
    };

    VkInstance vulkanInstance{VK_NULL_HANDLE};
    VK_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance));
    std::println("Vulkan instance created!");

    vkDestroyInstance(vulkanInstance, nullptr);
    std::println("Vulkan instance released");

    return EXIT_SUCCESS;
}
