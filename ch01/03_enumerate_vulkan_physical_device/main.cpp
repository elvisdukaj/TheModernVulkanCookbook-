#include <expected>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <unordered_map>
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

std::vector<VkExtensionProperties> enumerateExtensionsProperties() {
  uint32_t extensionsCount{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

  std::vector<VkExtensionProperties> extensionsProperties(extensionsCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionsProperties.data());

  return extensionsProperties;
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

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) {
  uint32_t physicalDevicesCount{0};
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr);

  std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data());

  return physicalDevices;
}

class PhysicalDevice {
public:
  PhysicalDevice(VkPhysicalDevice device, std::vector<VkExtensionProperties> extensions, VkSurfaceKHR surface)
      : device{device}, m_extensions{std::move<>(extensions)}, surface{surface} {}

  [[nodiscard]] inline VkPhysicalDevice getPhysicalDevice() const noexcept { return device; }
  [[nodiscard]] inline VkSurfaceKHR getSurface() const noexcept { return surface; }

private:
  VkPhysicalDevice device;
  std::vector<VkExtensionProperties> m_extensions;
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

  std::vector<PhysicalDevice> enumeratePhysicalDevices() {
    uint32_t physicalDevicesCount{0};
    vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDevicesCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDevicesCount, physicalDevices.data());

    // clang-format off
    auto result = physicalDevices
      | views::transform([this](VkPhysicalDevice device) -> PhysicalDevice {
         return PhysicalDevice{device, m_layerExtensions, m_surface};
        })
      | ranges::to<std::vector<PhysicalDevice>>();
    // clang-format on
    return result;
  }

private:
  Context(GLFWwindow* window, std::string_view applicationName, std::vector<std::string> requestedInstanceLayer,
          std::vector<std::string> requestedInstanceExtensions)
      : m_window{window}, m_applicationName{applicationName} {
    auto allInstanceLayers = enumerateInstanceLayerProperties();
    const auto isInstanceLayerRequired = [&requestedInstanceLayer](const VkLayerProperties& prop) {
      auto name = std::string{prop.layerName};
      return ranges::find(requestedInstanceLayer, name) != std::end(requestedInstanceLayer);
    };
    // clang-format off
    m_layerProperties = allInstanceLayers
      | views::filter(isInstanceLayerRequired)
      | ranges::to<std::vector<VkLayerProperties>>();
    // clang-format on

    auto allExtensions = enumerateExtensionsProperties();
    const auto isExtensionRequired = [&requestedInstanceExtensions](const VkExtensionProperties& prop) {
      auto name = std::string{prop.extensionName};
      return ranges::find(requestedInstanceExtensions, name) != std::end(requestedInstanceExtensions);
    };
    // clang-format off
    m_layerExtensions = allExtensions
      | views::filter(isExtensionRequired)
      | ranges::to<std::vector<VkExtensionProperties>>();
    // clang-format on
  }

  bool init() {
    // clang-format off
    auto layers = m_layerProperties
      | views::transform([](const VkLayerProperties& prop) -> const char*
        {
          return prop.layerName;
        })
      | ranges::to<std::vector<const char*>>();

      auto extensions = m_layerExtensions
        | views::transform([](const VkExtensionProperties & prop) -> const char*
          {
            return prop.extensionName;
          })
        | ranges::to<std::vector<const char*>>();
    // clang-format on

    const VkApplicationInfo applicationInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                            .pApplicationName = m_applicationName.data(),
                                            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                            .apiVersion = VK_API_VERSION_1_3};

    const VkInstanceCreateInfo instanceCreateInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(VK_USE_PLATFORM_METAL_EXT)
                                                  .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
                                                  .pApplicationInfo = &applicationInfo,
                                                  .enabledLayerCount = static_cast<uint32_t>(layers.size()),
                                                  .ppEnabledLayerNames = layers.data(),
                                                  .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                                                  .ppEnabledExtensionNames = extensions.data()};

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
    std::swap(m_vulkanInstance, rhs.m_vulkanInstance);
    std::swap(m_surface, rhs.m_surface);
  }

private:
  GLFWwindow* m_window = nullptr;
  std::string m_applicationName;
  std::vector<VkLayerProperties> m_layerProperties;
  std::vector<VkExtensionProperties> m_layerExtensions;
  VkInstance m_vulkanInstance = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};
} // namespace VulkanCore

int main() {
  const std::string applicationName = "01-03 Enumerate physical devices";

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(800, 600, applicationName.c_str(), nullptr, nullptr);
  glfwMakeContextCurrent(window);

  auto vulkanContext = VulkanCore::Context::create(window, applicationName, getRequestedInstanceLayers(),
                                                   getRequestedInstanceExtensions());

  if (!vulkanContext) {
    std::println("Unable to create the context: {}", vulkanContext.error());
    return EXIT_FAILURE;
  }

  auto physicalDevices = vulkanContext.value().enumeratePhysicalDevices();
  std::println("Found {} physical devices.", physicalDevices.size());

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
      glfwSetWindowShouldClose(window, true);
    }

    glfwSwapBuffers(window);
  }

  glfwTerminate();

  return EXIT_SUCCESS;
}
