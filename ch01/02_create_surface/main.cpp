#include <algorithm>
#include <expected>
#include <functional>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <vector>

#include <vulkan/vulkan.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

namespace ranges = std::ranges;
namespace views = std::ranges::views;

auto getRequestedInstanceLayers() -> std::vector<std::string> {
  return std::vector<std::string>{"VK_LAYER_KHRONOS_validation"};
}

auto getRequestedInstanceExtensions() -> std::vector<std::string> {
  return std::vector<std::string>{
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
}

namespace VulkanCore {
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

std::optional<VkSurfaceKHR> createVulkanSurface(GLFWwindow* window, VkInstance vulkanInstance) {
  VkSurfaceKHR surface;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  auto hwnd = glfwGetWin32Window(window);
  const VkWin32SurfaceCreateInfoKHR surfaceInfo{.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                                .hinstance = GetModuleHandleW(nullptr),
                                                .hwnd = reinterpret_cast<HWND>(hwnd)};

  const auto res = vkCreateWin32SurfaceKHR(vulkanInstance, &surfaceInfo, nullptr, &surface);
// #elif defined(VK_USE_PLATFORM_METAL_EXT)
//   auto layer = glfwCoc
#else
  const auto res = glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface);
#endif
  if (res == VK_SUCCESS)
    return surface;
  else
    return std::nullopt;
}

class PhysicalDevice {
public:
  PhysicalDevice(VkPhysicalDevice device, std::vector<VkExtensionProperties> extensions, VkSurfaceKHR surface)
      : device{device}, extension{std::move(extensions)}, surface{surface} {}

  [[nodiscard]] inline VkPhysicalDevice getPhysicalDevice() const noexcept { return device; }

  [[nodiscard]] inline VkSurfaceKHR getSurface() const noexcept { return surface; }

private:
  VkPhysicalDevice device;
  std::vector<VkExtensionProperties> extension;
  VkSurfaceKHR surface;
};

class Context {
public:
  static std::expected<Context, std::string> create(GLFWwindow* window, std::string_view applicationName,
                                                    std::vector<std::string> requestedInstanceLayer,
                                                    std::vector<std::string> requestedInstanceExtensions) {

    Context context{window, applicationName, std::move(requestedInstanceLayer), std::move(requestedInstanceExtensions)};

    if (context.init())
      return context;
    else
      return std::unexpected(std::string{"Failed to init the vulkan context"});
  }

  ~Context() {
    if (m_vulkanInstance == VK_NULL_HANDLE)
      return;

    vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
    vkDestroyInstance(m_vulkanInstance, nullptr);
    m_vulkanInstance = VK_NULL_HANDLE;
  }

  Context& operator=(const Context&) = delete;

  Context(const Context&) = delete;

  Context(Context&& rhs) noexcept {
    swap(rhs);
    rhs.m_vulkanInstance = VK_NULL_HANDLE;
    rhs.m_surface = VK_NULL_HANDLE;
  }

  Context& operator=(Context&& rhs) noexcept {
    Context tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

private:
  Context(GLFWwindow* window, std::string_view applicationName, std::vector<std::string> requestedInstanceLayer,
          std::vector<std::string> requestedInstanceExtensions)
      : m_window{window}, m_applicationName{applicationName},
        m_requestedInstanceLayer{std::move(requestedInstanceLayer)},
        m_requestedInstanceExtensions(std::move(requestedInstanceExtensions)) {
    m_layersView = m_requestedInstanceLayer | views::transform(std::mem_fn(&std::string::c_str)) |
                   ranges::to<std::vector<const char*>>();
    m_extensionsView = m_requestedInstanceExtensions | views::transform(std::mem_fn(&std::string::c_str)) |
                       ranges::to<std::vector<const char*>>();
  }

  bool init() {
    const VkApplicationInfo applicationInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                            .pApplicationName = m_applicationName.data(),
                                            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                            .apiVersion = VK_API_VERSION_1_3};

    const VkInstanceCreateInfo instanceCreateInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(VK_USE_PLATFORM_METAL_EXT)
                                                  .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
                                                  .pApplicationInfo = &applicationInfo,
                                                  .enabledLayerCount = static_cast<uint32_t>(m_layersView.size()),
                                                  .ppEnabledLayerNames = m_layersView.data(),
                                                  .enabledExtensionCount =
                                                      static_cast<uint32_t>(m_extensionsView.size()),
                                                  .ppEnabledExtensionNames = m_extensionsView.data()};

    const auto res = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vulkanInstance);
    if (res != VK_SUCCESS)
      return false;

    auto surface = createVulkanSurface(m_window, m_vulkanInstance);
    if (!surface.has_value())
      return false;

    m_surface = surface.value();
    return true;
  }

  void swap(Context& rhs) {
    std::swap(m_window, rhs.m_window);
    std::swap(m_applicationName, rhs.m_applicationName);
    std::swap(m_requestedInstanceLayer, rhs.m_requestedInstanceLayer);
    std::swap(m_requestedInstanceExtensions, rhs.m_requestedInstanceExtensions);
    std::swap(m_layersView, rhs.m_layersView);
    std::swap(m_extensionsView, rhs.m_extensionsView);
    std::swap(m_vulkanInstance, rhs.m_vulkanInstance);
    std::swap(m_surface, rhs.m_surface);
  }

  std::vector<PhysicalDevice> enumeratePhysicalDevices() {
    uint32_t physicalDevicesCount{0};
    vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDevicesCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDevicesCount, physicalDevices.data());

    std::vector<PhysicalDevice> result;
    return result;
  }

private:
  GLFWwindow* m_window = nullptr;
  std::string m_applicationName;
  std::vector<std::string> m_requestedInstanceLayer;
  std::vector<std::string> m_requestedInstanceExtensions;
  std::vector<const char*> m_layersView;
  std::vector<const char*> m_extensionsView;
  VkInstance m_vulkanInstance = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};
} // namespace VulkanCore

int main() {
  const auto requestedInstanceLayers = getRequestedInstanceLayers();
  const auto requestedInstanceExtensions = getRequestedInstanceExtensions();

  const auto isInstanceLayerRequired = [&requestedInstanceLayers](const std::string& name) {
    return ranges::find(requestedInstanceLayers, name) != std::end(requestedInstanceLayers);
  };

  const auto isExtensionRequired = [&requestedInstanceExtensions](const std::string& name) {
    return ranges::find(requestedInstanceExtensions, name) != std::end(requestedInstanceExtensions);
  };

  const auto availableLayers = VulkanCore::getAvailableInstanceLayersName();
  const auto availableExtensions = VulkanCore::getLayerExtensionsName();

  const auto enabledInstanceLayers =
      availableLayers | views::filter(isInstanceLayerRequired) | ranges::to<std::vector<std::string>>();

  const auto enabledInstanceExtensions =
      availableExtensions | views::filter(isExtensionRequired) | ranges::to<std::vector<std::string>>();

  const std::string applicationName = "01-02 Create Vulkan Surface";

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(800, 600, applicationName.c_str(), nullptr, nullptr);
  glfwMakeContextCurrent(window);

  auto vulkanContext =
      VulkanCore::Context::create(window, applicationName, enabledInstanceLayers, enabledInstanceExtensions);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    glfwSwapBuffers(window);
  }

  glfwTerminate();

  return EXIT_SUCCESS;
}
