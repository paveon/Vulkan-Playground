#define GLFW_INCLUDE_VULKAN

#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>
#include <limits>
#include <chrono>
#include <thread>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include "mathlib.h"
#include "utils.h"

#define TIME_NOW std::chrono::steady_clock::now()

#define VK_CHECK(call) assert(call == VK_SUCCESS)


struct SwapchainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};


struct UniformBufferObject {
   alignas(8) math::vec2 foo;
   alignas(16) math::mat4 model;
   alignas(16) math::mat4 view;
   alignas(16) math::mat4 proj;
};


struct Vertex {
   math::vec2 pos;
   math::vec3 color;

   static VkVertexInputBindingDescription getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription = {};
      bindingDescription.binding = 0;
      bindingDescription.stride = sizeof(Vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
   }

   static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
      std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[0].offset = 0;

      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset = sizeof(pos);

      return attributeDescriptions;
   }
};


const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}
};


using VertexIndex = uint16_t;

const std::vector<VertexIndex> indices = {
        0, 1, 2, 2, 3, 0
};


class Renderer {
public:
   const uint32_t WIDTH = 800;
   const uint32_t HEIGHT = 600;

   ~Renderer() {
      if (m_Device) {
         for (uint32_t i = 0; i < m_AcquireSemaphores.size(); i++) {
            if (m_AcquireSemaphores[i] != nullptr)
               vkDestroySemaphore(m_Device, m_AcquireSemaphores[i], nullptr);
            if (m_ReleaseSemaphores[i] != nullptr)
               vkDestroySemaphore(m_Device, m_ReleaseSemaphores[i], nullptr);
            if (m_Fences[i] != nullptr)
               vkDestroyFence(m_Device, m_Fences[i], nullptr);
         }

         for (uint32_t i = 0; i < m_SwapchainFramebuffers.size(); i++) {
            if (m_SwapchainFramebuffers[i])
               vkDestroyFramebuffer(m_Device, m_SwapchainFramebuffers[i], nullptr);
            if (m_SwapchainImageViews[i])
               vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
            if (m_UniformBuffers[i])
               vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
            if (m_UniformMemory[i])
               vkFreeMemory(m_Device, m_UniformMemory[i], nullptr);
         }

         if (m_DescriptorPool)
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
         if (m_IndexBuffer)
            vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
         if (m_IndexBufferMemory)
            vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
         if (m_VertexBuffer)
            vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
         if (m_VertexBufferMemory)
            vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
         if (m_CommandPool)
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
         if (m_TransferCommandPool)
            vkDestroyCommandPool(m_Device, m_TransferCommandPool, nullptr);
         if (m_DescriptorSetLayout)
            vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
         if (m_GraphicsPipeline)
            vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
         if (m_GraphicsPipeline)
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
         if (m_RenderPass)
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
         if (m_Swapchain)
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

         vkDestroyDevice(m_Device, nullptr);
      }

      if (m_Instance) {
         if (m_Surface)
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
         if (m_EnableValidationLayers && m_DebugMessenger)
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

         vkDestroyInstance(m_Instance, nullptr);
      }

      glfwDestroyWindow(m_Window);
      glfwTerminate();
   }

   void run() {
      initWindow();
      initVulkan();
      mainLoop();
   }

private:
   enum QueueType {
      QUEUE_GRAPHICS,
      QUEUE_PRESENT,
      QUEUE_TRANSFER,
      QUEUE_Count
   };


   const char* vertShaderPath = "shaders/triangle.vert.spv";
   const char* fragShaderPath = "shaders/triangle.frag.spv";

   GLFWwindow* m_Window = nullptr;
   VkInstance m_Instance = nullptr;
   VkDebugUtilsMessengerEXT m_DebugMessenger;
   VkPhysicalDevice m_PhysicalDevice = nullptr;
   VkDevice m_Device = nullptr;
   VkSurfaceKHR m_Surface = nullptr;
   VkSwapchainKHR m_Swapchain = nullptr;
   VkRenderPass m_RenderPass = nullptr;
   VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;
   VkPipelineLayout m_PipelineLayout = nullptr;
   VkPipeline m_GraphicsPipeline = nullptr;
   VkCommandPool m_CommandPool = nullptr;
   VkCommandPool m_TransferCommandPool = nullptr;
   VkFormat m_SwapchainImageFormat;
   VkExtent2D m_SwapchainExtent;

   VkBuffer m_VertexBuffer = nullptr;
   VkDeviceMemory m_VertexBufferMemory = nullptr;
   VkBuffer m_IndexBuffer = nullptr;
   VkDeviceMemory m_IndexBufferMemory = nullptr;
   VkDescriptorPool m_DescriptorPool = nullptr;

   std::vector<VkDescriptorSet> m_DescriptorSets;
   std::vector<VkBuffer> m_UniformBuffers;
   std::vector<VkDeviceMemory> m_UniformMemory;

   std::vector<VkImage> m_SwapchainImages;
   std::vector<VkImageView> m_SwapchainImageViews;
   std::vector<VkFramebuffer> m_SwapchainFramebuffers;
   std::vector<VkCommandBuffer> m_CmdBuffers;
   std::vector<VkCommandBuffer> m_TransferCmdBuffers;
   std::vector<VkSemaphore> m_AcquireSemaphores;
   std::vector<VkSemaphore> m_ReleaseSemaphores;
   std::vector<VkFence> m_Fences;

   VkQueue m_Queues[QUEUE_Count] = {};
   uint32_t m_QueueIndices[QUEUE_Count] = {};

   const size_t MAX_FRAMES_IN_FLIGHT = 2;

   size_t m_FrameIndex = 0;
   bool m_FramebufferResized = false;

   const bool m_EnableValidationLayers = true;
   const std::vector<const char*> m_ValidationLayers = {
           "VK_LAYER_KHRONOS_validation",
           "VK_LAYER_LUNARG_monitor"
   };

   const std::vector<const char*> m_DeviceExtensions = {
           VK_KHR_SWAPCHAIN_EXTENSION_NAME
   };


   VkDebugUtilsMessengerEXT setupDebugMessenger() {
      VkDebugUtilsMessengerEXT debugMessenger;
      VkDebugUtilsMessengerCreateInfoEXT createInfo = createDebugMsgInfo(debugCallback, nullptr);
      VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &debugMessenger));

      return debugMessenger;
   }


   std::vector<const char*> getRequiredExtensions() {
      uint32_t extensionCount;
      const char** glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

      std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);

      if (m_EnableValidationLayers) {
         extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }

      return extensions;
   }


   bool checkValidationLayerSupport() {
      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

      std::vector<VkLayerProperties> availableLayers(layerCount);
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

      for (const char* layerName : m_ValidationLayers) {
         bool layerFound = false;

         for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
               layerFound = true;
               break;
            }
         }

         if (!layerFound) { return false; }
      }

      return true;
   }


   static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
      (void) width;
      (void) height;
      auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
      renderer->m_FramebufferResized = true;
   }


   void initWindow() {
      assert(glfwInit());
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

      m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Renderer", nullptr, nullptr);

      glfwSetWindowUserPointer(m_Window, this);
      glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
   }

   VkInstance createInstance() {
      if (m_EnableValidationLayers && !checkValidationLayerSupport()) {
         throw std::runtime_error("[Vulkan] validation layers requested, but not available!");
      }

      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "Renderer";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_1;

      VkInstanceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &appInfo;
      auto extensions = getRequiredExtensions();
      createInfo.enabledExtensionCount = extensions.size();
      createInfo.ppEnabledExtensionNames = extensions.data();

      VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
      if (m_EnableValidationLayers) {
         createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
         createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
         debugCreateInfo = createDebugMsgInfo(debugCallback, nullptr);
         createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
      }

      VkInstance instance;
      VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
      VK_CHECK(result);

      return instance;
   }


   VkPhysicalDevice pickPhysicalDevice() {
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
      if (deviceCount == 0) { throw std::runtime_error("No GPU with Vulkan support!"); }
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

      VkPhysicalDevice fallback = nullptr;
      VkPhysicalDeviceProperties deviceProperties;
      VkPhysicalDeviceFeatures deviceFeatures;
      for (const auto& device : devices) {
         vkGetPhysicalDeviceProperties(device, &deviceProperties);
         vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

         if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            std::cout << "[Vulkan] Picking discrete GPU: " << deviceProperties.deviceName << std::endl;
            return device;
         }
         else if (!fallback) { fallback = device; }
      }

      if (!fallback) {
         throw std::runtime_error("No suitable GPU!");
      }

      vkGetPhysicalDeviceProperties(fallback, &deviceProperties);
      std::cout << "[Vulkan] Picking fallback GPU: " << deviceProperties.deviceName << std::endl;
      return fallback;
   }


   VkDevice createLogicalDevice() {
      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

      std::set<uint32_t> uniqueQueueFamilies;
      bool graphicsFamilyExists = false;
      bool presentFamilyExists = false;
      bool transferFamilyExists = false;
      for (uint32_t i = 0; i < queueFamilyCount; i++) {
         if (queueFamilies[i].queueCount > 0) {
            auto flags = queueFamilies[i].queueFlags;
            if (!graphicsFamilyExists && flags & VK_QUEUE_GRAPHICS_BIT) {
               m_QueueIndices[QUEUE_GRAPHICS] = i;
               uniqueQueueFamilies.insert(i);
               graphicsFamilyExists = true;
            }

            if (!transferFamilyExists && flags & VK_QUEUE_TRANSFER_BIT) {
               m_QueueIndices[QUEUE_TRANSFER] = i;
               uniqueQueueFamilies.insert(i);
               transferFamilyExists = true;
            }

            if (!presentFamilyExists) {
               VkBool32 presentSupport = false;
               vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);
               if (presentSupport) {
                  m_QueueIndices[QUEUE_PRESENT] = i;
                  uniqueQueueFamilies.insert(i);
                  presentFamilyExists = true;
               }
            }
         }

         if (presentFamilyExists && graphicsFamilyExists && transferFamilyExists)
            break;
      }

      if (!graphicsFamilyExists) {
         throw std::runtime_error("GPU does not support graphics command queue!");
      }
      else if (!presentFamilyExists) {
         throw std::runtime_error("GPU does not support present command queue!");
      }
      else if (!transferFamilyExists) {
         throw std::runtime_error("GPU does not support transfer command queue!");
      }


      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

      std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
      for (const auto& extension : availableExtensions) {
         requiredExtensions.erase(extension.extensionName);
      }
      if (!requiredExtensions.empty()) {
         for (const auto& missing : requiredExtensions) {
            std::cerr << "[Vulkan] Missing extension: " << missing << std::endl;
         }
         throw std::runtime_error("GPU does not support required extensions!");
      }


      SwapchainSupportDetails swapChainSupport = querySwapchainSupport(m_PhysicalDevice);
      if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
         throw std::runtime_error("GPU does not support required swapchain features!");
      }


      std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
      float queuePriority = 1.0f;
      for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
         VkDeviceQueueCreateInfo queueCreateInfo = {};
         queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
         queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
         queueCreateInfo.queueCount = 1;
         queueCreateInfo.pQueuePriorities = &queuePriority;
         queueCreateInfos.push_back(queueCreateInfo);
      }

      VkPhysicalDeviceFeatures deviceFeatures = {};
      VkDeviceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
      createInfo.pQueueCreateInfos = queueCreateInfos.data();
      createInfo.pEnabledFeatures = &deviceFeatures;
      createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
      createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

      VkDevice device;
      if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
         throw std::runtime_error("failed to create logical device!");
      }

      for (size_t i = 0; i < QUEUE_Count; i++) {
         vkGetDeviceQueue(device, m_QueueIndices[i], 0, &m_Queues[i]);
      }

      return device;
   }


   VkSurfaceKHR createSurface() {
      VkSurfaceKHR surface;
      if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface) != VK_SUCCESS) {
         throw std::runtime_error("failed to create window surface!");
      }
      return surface;
   }


   SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) {
      SwapchainSupportDetails details;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
      if (formatCount != 0) {
         details.formats.resize(formatCount);
         vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
      }

      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
      if (presentModeCount != 0) {
         details.presentModes.resize(presentModeCount);
         vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
      }

      return details;
   }


   VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
      for (const auto& availableFormat : availableFormats) {
         if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
         }
      }

      return availableFormats[0];
   }


   VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
      VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

//      for (const auto& availablePresentMode : availablePresentModes) {
//         if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
//            return availablePresentMode;
//         }
//         else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
//            bestMode = availablePresentMode;
//         }
//      }

      return bestMode;
   }


   VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
      if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
         return capabilities.currentExtent;
      }
      else {
         int width, height;
         glfwGetFramebufferSize(m_Window, &width, &height);

         VkExtent2D actualExtent = {
                 static_cast<uint32_t>(width),
                 static_cast<uint32_t>(height)
         };
         actualExtent.width = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, actualExtent.width));
         actualExtent.height = std::max(capabilities.minImageExtent.height,
                                        std::min(capabilities.maxImageExtent.height, actualExtent.height));

         return actualExtent;
      }
   }


   VkSwapchainKHR createSwapChain() {
      SwapchainSupportDetails swapchainSupport = querySwapchainSupport(m_PhysicalDevice);

      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
      VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
      m_SwapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);
      m_SwapchainImageFormat = surfaceFormat.format;

      uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
      if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
         imageCount = swapchainSupport.capabilities.maxImageCount;
      }

      VkSwapchainCreateInfoKHR createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      createInfo.surface = m_Surface;
      createInfo.minImageCount = imageCount;
      createInfo.imageFormat = surfaceFormat.format;
      createInfo.imageColorSpace = surfaceFormat.colorSpace;
      createInfo.imageExtent = m_SwapchainExtent;
      createInfo.imageArrayLayers = 1;
      createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
      createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      createInfo.presentMode = presentMode;
      createInfo.clipped = VK_TRUE;
      createInfo.oldSwapchain = nullptr;

      if (m_QueueIndices[QUEUE_GRAPHICS] != m_QueueIndices[QUEUE_PRESENT]) {
         uint32_t queueIndices[] = {
                 m_QueueIndices[QUEUE_GRAPHICS],
                 m_QueueIndices[QUEUE_PRESENT]
         };

         createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
         createInfo.queueFamilyIndexCount = 2;
         createInfo.pQueueFamilyIndices = queueIndices;
      }
      else {
         createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
         createInfo.queueFamilyIndexCount = 0;
         createInfo.pQueueFamilyIndices = nullptr;
      }

      VkSwapchainKHR swapchain;
      if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
         throw std::runtime_error("failed to create swap chain!");
      }

      vkGetSwapchainImagesKHR(m_Device, swapchain, &imageCount, nullptr);
      m_SwapchainImages.resize(imageCount);
      vkGetSwapchainImagesKHR(m_Device, swapchain, &imageCount, m_SwapchainImages.data());

      return swapchain;
   }


   void createImageViews() {
      m_SwapchainImageViews.resize(m_SwapchainImages.size());

      VkImageViewCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = m_SwapchainImageFormat;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
         createInfo.image = m_SwapchainImages[i];

         if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
         }
      }
   }


   class ShaderModule {
   public:
      ShaderModule(VkDevice device, const std::string& filename) : owner(device) {
         std::vector<char> shaderCode = readFile(filename);
         codeSize = shaderCode.size();

         VkShaderModuleCreateInfo createInfo = {};
         createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
         createInfo.codeSize = codeSize;
         createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

         if (vkCreateShaderModule(owner, &createInfo, nullptr, &data) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
         }
      }

      ~ShaderModule() {
         vkDestroyShaderModule(owner, data, nullptr);
      }

      operator VkShaderModule() const { return data; }

      uint32_t size() const { return codeSize; }

   private:
      VkDevice owner = nullptr;
      VkShaderModule data = nullptr;
      uint32_t codeSize;
   };


   VkPipelineLayout createGraphicsPipelineLayout() {
      VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
      pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutInfo.setLayoutCount = 1; // Optional
      pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional
      pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
      pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

      VkPipelineLayout layout;
      if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
         throw std::runtime_error("failed to create pipeline layout!");
      }

      return layout;
   }


   VkPipeline createGraphicsPipeline() {
      ShaderModule vertShader(m_Device, vertShaderPath);
      ShaderModule fragShader(m_Device, fragShaderPath);
      std::cout << "[Vulkan] Loaded vertex shader bytecode, size: "
                << vertShader.size() << " bytes" << std::endl;
      std::cout << "[Vulkan] Loaded fragment shader bytecode, size: "
                << fragShader.size() << " bytes" << std::endl;

      VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
      vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      vertShaderStageInfo.module = vertShader;
      vertShaderStageInfo.pName = "main";

      VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
      fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      fragShaderStageInfo.module = fragShader;
      fragShaderStageInfo.pName = "main";

      VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

      auto bindingDescription = Vertex::getBindingDescription();
      auto attributeDescriptions = Vertex::getAttributeDescriptions();

      VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
      vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vertexInputInfo.vertexBindingDescriptionCount = 1;
      vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
      vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
      vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

      VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
      inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      inputAssembly.primitiveRestartEnable = VK_FALSE;

      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(m_SwapchainExtent.width);
      viewport.height = static_cast<float>(m_SwapchainExtent.height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor = {};
      scissor.offset = {0, 0};
      scissor.extent = m_SwapchainExtent;

      VkPipelineViewportStateCreateInfo viewportState = {};
      viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      viewportState.viewportCount = 1;
      viewportState.pViewports = &viewport;
      viewportState.scissorCount = 1;
      viewportState.pScissors = &scissor;

      VkPipelineRasterizationStateCreateInfo rasterizer = {};
      rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rasterizer.depthClampEnable = VK_FALSE; //Discard fragments beyond near/far planes instead of clamping
      rasterizer.rasterizerDiscardEnable = VK_FALSE; //Do not discard fragments
      rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
      rasterizer.lineWidth = 1.0f; //Single fragment line width
      rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //Back-face culling
      rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Front-facing polygons have vertices in clockwise order
      rasterizer.depthBiasEnable = VK_FALSE;
      rasterizer.depthBiasConstantFactor = 0.0f; // Optional
      rasterizer.depthBiasClamp = 0.0f; // Optional
      rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

      VkPipelineMultisampleStateCreateInfo multisampling = {};
      multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      multisampling.sampleShadingEnable = VK_FALSE;
      multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      multisampling.minSampleShading = 1.0f; // Optional
      multisampling.pSampleMask = nullptr; // Optional
      multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
      multisampling.alphaToOneEnable = VK_FALSE; // Optional

      VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;
      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
      colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
      colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

      //Alpha blending
//      colorBlendAttachment.blendEnable = VK_TRUE;
//      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//      colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//      colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

      VkPipelineColorBlendStateCreateInfo colorBlending = {};
      colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlending.logicOpEnable = VK_FALSE;
      colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
      colorBlending.attachmentCount = 1;
      colorBlending.pAttachments = &colorBlendAttachment;
      colorBlending.blendConstants[0] = 0.0f; // Optional
      colorBlending.blendConstants[1] = 0.0f; // Optional
      colorBlending.blendConstants[2] = 0.0f; // Optional
      colorBlending.blendConstants[3] = 0.0f; //

      VkDynamicState dynamicStates[] = {
              VK_DYNAMIC_STATE_VIEWPORT,
              VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamicState = {};
      dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dynamicState.dynamicStateCount = 2;
      dynamicState.pDynamicStates = dynamicStates;


      VkGraphicsPipelineCreateInfo pipelineInfo = {};
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = 2;
      pipelineInfo.pStages = shaderStages;
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizer;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pDepthStencilState = nullptr; // Optional
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = &dynamicState;
      pipelineInfo.layout = m_PipelineLayout;
      pipelineInfo.renderPass = m_RenderPass;
      pipelineInfo.subpass = 0;
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
      pipelineInfo.basePipelineIndex = -1; // Optional

      VkPipeline pipeline;
      if (vkCreateGraphicsPipelines(m_Device, nullptr, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
         throw std::runtime_error("failed to create graphics pipeline!");
      }

      return pipeline;
   }


   VkRenderPass createRenderPass() {
      VkAttachmentDescription colorAttachment = {};
      colorAttachment.format = m_SwapchainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference colorAttachmentRef = {};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;

      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = 1;
      renderPassInfo.pAttachments = &colorAttachment;
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 1;
      renderPassInfo.pDependencies = &dependency;

      VkRenderPass renderPass;
      if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
         throw std::runtime_error("failed to create render pass!");
      }

      return renderPass;
   }


   void createFramebuffers() {
      m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

      VkFramebufferCreateInfo framebufferInfo = {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_RenderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.width = m_SwapchainExtent.width;
      framebufferInfo.height = m_SwapchainExtent.height;
      framebufferInfo.layers = 1;

      for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
         framebufferInfo.pAttachments = &m_SwapchainImageViews[i];

         if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
         }
      }
   }


   VkCommandPool createCommandPool(QueueType queueType, VkCommandPoolCreateFlags flags = 0) {
      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = m_QueueIndices[queueType];
      poolInfo.flags = flags;

      VkCommandPool pool;
      if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
         throw std::runtime_error("failed to create command pool!");
      }

      return pool;
   }


   void createCommandBuffers() {
      m_CmdBuffers.resize(m_SwapchainFramebuffers.size());
      m_TransferCmdBuffers.resize(m_SwapchainFramebuffers.size());

      VkCommandBufferAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = m_CommandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = static_cast<uint32_t>(m_SwapchainFramebuffers.size());

      if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CmdBuffers.data()) != VK_SUCCESS) {
         throw std::runtime_error("failed to allocate command buffers!");
      }

      allocInfo.commandPool = m_TransferCommandPool;
      if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_TransferCmdBuffers.data()) != VK_SUCCESS) {
         throw std::runtime_error("failed to allocate command buffers!");
      }


      VkDeviceSize bufferOffsets[] = {0};
      VkClearColorValue colorValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
      VkClearValue clearColor = {colorValue};

      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(m_SwapchainExtent.width);
      viewport.height = static_cast<float>(m_SwapchainExtent.height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor = {};
      scissor.offset = {0, 0};
      scissor.extent = m_SwapchainExtent;

      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      beginInfo.pInheritanceInfo = nullptr; // Optional

      VkRenderPassBeginInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = m_RenderPass;
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = m_SwapchainExtent;
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      for (size_t i = 0; i < m_CmdBuffers.size(); i++) {
         if (vkBeginCommandBuffer(m_CmdBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
         }

         VkImageMemoryBarrier renderBeginBarrier = imageBarrier(m_SwapchainImages[i], 0,
                                                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

         vkCmdPipelineBarrier(m_CmdBuffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT,
                              0, nullptr, 0, nullptr, 1, &renderBeginBarrier);

         vkCmdSetViewport(m_CmdBuffers[i], 0, 1, &viewport);
         vkCmdSetScissor(m_CmdBuffers[i], 0, 1, &scissor);

         renderPassInfo.framebuffer = m_SwapchainFramebuffers[i];

         vkCmdBeginRenderPass(m_CmdBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

         vkCmdBindPipeline(m_CmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

         vkCmdBindVertexBuffers(m_CmdBuffers[i], 0, 1, &m_VertexBuffer, bufferOffsets);

         vkCmdBindIndexBuffer(m_CmdBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

         //vkCmdDraw(m_CmdBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

         vkCmdBindDescriptorSets(m_CmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

         vkCmdDrawIndexed(m_CmdBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

         vkCmdEndRenderPass(m_CmdBuffers[i]);


         VkImageMemoryBarrier renderEndBarrier = imageBarrier(m_SwapchainImages[i],
                                                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
                                                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

         vkCmdPipelineBarrier(m_CmdBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
                              0, nullptr, 0, nullptr, 1, &renderEndBarrier);

         if (vkEndCommandBuffer(m_CmdBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
         }
      }
   }


   void createSyncObjects() {
      m_AcquireSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      m_ReleaseSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      m_Fences.resize(MAX_FRAMES_IN_FLIGHT);

      VkSemaphoreCreateInfo semaphoreInfo = {};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
         if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_AcquireSemaphores[i]) != VK_SUCCESS ||
             vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ReleaseSemaphores[i]) != VK_SUCCESS ||
             vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores for a frame!");
         }
      }
   }


   VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                     VkImageLayout oldLayout, VkImageLayout newLayout) {
      VkImageMemoryBarrier result = {};
      result.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      result.srcAccessMask = srcAccessMask;
      result.dstAccessMask = dstAccessMask;
      result.oldLayout = oldLayout;
      result.newLayout = newLayout;
      result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      result.image = image;
      result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

      return result;
   }


   uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
      VkPhysicalDeviceMemoryProperties memProperties;
      vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

      for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
         if ((typeFilter & (1UL << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
         }
      }

      throw std::runtime_error("failed to find suitable memory type!");
   }


   VkDeviceMemory getDeviceMemory(VkBuffer& buffer, VkMemoryPropertyFlags properties) {
      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

      VkMemoryAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

      VkDeviceMemory bufferMemory;
      if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
         throw std::runtime_error("failed to allocate buffer memory!");
      }

      return bufferMemory;
   }


   //Instead of using VK_SHARING_MODE_CONCURRENT, it should be better to use a memory barrier
   //to release ownership from the graphicQueue to the transferQueue, make the copy, and release
   //ownership from the transferQueue to the graphicQueue.
   VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
      VkBufferCreateInfo bufferInfo = {};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = size;
      bufferInfo.usage = usage;
      if (m_QueueIndices[QUEUE_GRAPHICS] != m_QueueIndices[QUEUE_TRANSFER]) {
         uint32_t queueIndices[] = {
                 m_QueueIndices[QUEUE_GRAPHICS],
                 m_QueueIndices[QUEUE_TRANSFER]
         };

         bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
         bufferInfo.queueFamilyIndexCount = 2;
         bufferInfo.pQueueFamilyIndices = queueIndices;
      }
      else {
         bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
         bufferInfo.queueFamilyIndexCount = 0;
         bufferInfo.pQueueFamilyIndices = nullptr;
      }

      VkBuffer buffer;
      if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
         throw std::runtime_error("failed to create buffer!");
      }

      return buffer;
   }


   void createVertexBuffer() {
      VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
      VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

      VkBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      VkDeviceMemory stagingBufferMemory = getDeviceMemory(stagingBuffer, memoryFlags);
      vkBindBufferMemory(m_Device, stagingBuffer, stagingBufferMemory, 0);

      void* data;
      vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, vertices.data(), bufferSize);
      vkUnmapMemory(m_Device, stagingBufferMemory);

      VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      m_VertexBuffer = createBuffer(bufferSize, bufferUsage);
      m_VertexBufferMemory = getDeviceMemory(m_VertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkBindBufferMemory(m_Device, m_VertexBuffer, m_VertexBufferMemory, 0);

      copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

      vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
      vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
   }


   void createIndexBuffer() {
      VkDeviceSize bufferSize = sizeof(VertexIndex) * indices.size();
      VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


      VkBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      VkDeviceMemory stagingBufferMemory = getDeviceMemory(stagingBuffer, memoryFlags);
      vkBindBufferMemory(m_Device, stagingBuffer, stagingBufferMemory, 0);

      void* data;
      vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, indices.data(), bufferSize);
      vkUnmapMemory(m_Device, stagingBufferMemory);

      VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      m_IndexBuffer = createBuffer(bufferSize, bufferUsage);
      m_IndexBufferMemory = getDeviceMemory(m_IndexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkBindBufferMemory(m_Device, m_IndexBuffer, m_IndexBufferMemory, 0);

      copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

      vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
      vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
   }


   void createUniformBuffers() {
      VkDeviceSize bufferSize = sizeof(UniformBufferObject);

      m_UniformBuffers.resize(m_SwapchainImages.size());
      m_UniformMemory.resize(m_SwapchainImages.size());

      VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

      for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
         m_UniformBuffers[i] = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
         m_UniformMemory[i] = getDeviceMemory(m_UniformBuffers[i], memoryFlags);
         vkBindBufferMemory(m_Device, m_UniformBuffers[i], m_UniformMemory[i], 0);
      }
   }


   void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
      VkCommandBufferAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandPool = m_TransferCommandPool;
      allocInfo.commandBufferCount = 1;

      VkCommandBuffer commandBuffer;
      vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      vkBeginCommandBuffer(commandBuffer, &beginInfo);

      VkBufferCopy copyRegion = {};
      copyRegion.srcOffset = 0; // Optional
      copyRegion.dstOffset = 0; // Optional
      copyRegion.size = size;
      vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

      vkEndCommandBuffer(commandBuffer);

      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;

      vkQueueSubmit(m_Queues[QUEUE_TRANSFER], 1, &submitInfo, nullptr);
      vkQueueWaitIdle(m_Queues[QUEUE_TRANSFER]);

      vkFreeCommandBuffers(m_Device, m_TransferCommandPool, 1, &commandBuffer);
   }


   void createDescriptorSetLayout() {
      VkDescriptorSetLayoutBinding uboLayoutBinding = {};
      uboLayoutBinding.binding = 0;
      uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uboLayoutBinding.descriptorCount = 1;
      uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

      VkDescriptorSetLayoutCreateInfo layoutInfo = {};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = 1;
      layoutInfo.pBindings = &uboLayoutBinding;

      if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
         throw std::runtime_error("failed to create descriptor set layout!");
      }
   }


   void createDescriptorPool() {
      VkDescriptorPoolSize poolSize = {};
      poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      poolSize.descriptorCount = static_cast<uint32_t>(m_SwapchainImages.size());

      VkDescriptorPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = 1;
      poolInfo.pPoolSizes = &poolSize;
      poolInfo.maxSets = static_cast<uint32_t>(m_SwapchainImages.size());

      if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
         throw std::runtime_error("failed to create descriptor pool!");
      }
   }


   void createDescriptorSets() {
      std::vector<VkDescriptorSetLayout> layouts(m_SwapchainImages.size(), m_DescriptorSetLayout);
      VkDescriptorSetAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = m_DescriptorPool;
      allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
      allocInfo.pSetLayouts = layouts.data();

      m_DescriptorSets.resize(layouts.size());
      if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
         throw std::runtime_error("failed to allocate descriptor sets!");
      }

      for (size_t i = 0; i < m_DescriptorSets.size(); i++) {
         VkDescriptorBufferInfo bufferInfo = {};
         bufferInfo.buffer = m_UniformBuffers[i];
         bufferInfo.offset = 0;
         bufferInfo.range = sizeof(UniformBufferObject);

         VkWriteDescriptorSet descriptorWrite = {};
         descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         descriptorWrite.dstSet = m_DescriptorSets[i];
         descriptorWrite.dstBinding = 0;
         descriptorWrite.dstArrayElement = 0;
         descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
         descriptorWrite.descriptorCount = 1;
         descriptorWrite.pBufferInfo = &bufferInfo;
         descriptorWrite.pImageInfo = nullptr; // Optional
         descriptorWrite.pTexelBufferView = nullptr; //

         vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
      }
   }


   void initVulkan() {
      m_Instance = createInstance();
      if (m_EnableValidationLayers)
         m_DebugMessenger = setupDebugMessenger();

      m_Surface = createSurface();
      m_PhysicalDevice = pickPhysicalDevice();
      m_Device = createLogicalDevice();
      m_Swapchain = createSwapChain();
      createImageViews();
      m_RenderPass = createRenderPass();
      createDescriptorSetLayout();
      m_PipelineLayout = createGraphicsPipelineLayout();
      m_GraphicsPipeline = createGraphicsPipeline();
      createFramebuffers();
      m_CommandPool = createCommandPool(QUEUE_GRAPHICS);
      m_TransferCommandPool = createCommandPool(QUEUE_TRANSFER, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
      createVertexBuffer();
      createIndexBuffer();
      createUniformBuffers();
      createDescriptorPool();
      createDescriptorSets();
      createCommandBuffers();
      createSyncObjects();
   }


   void cleanupSwapchain() {
      if (m_Device) {
         if (m_DescriptorPool)
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

         if (m_CommandPool)
            vkFreeCommandBuffers(m_Device, m_CommandPool, m_CmdBuffers.size(), m_CmdBuffers.data());

         //vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
         //vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
         if (m_RenderPass)
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

         for (uint32_t i = 0; i < m_SwapchainImages.size(); i++) {
            if (m_UniformBuffers[i])
               vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
            if (m_UniformMemory[i])
               vkFreeMemory(m_Device, m_UniformMemory[i], nullptr);
            if (m_SwapchainImageViews[i])
               vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
            if (m_SwapchainFramebuffers[i])
               vkDestroyFramebuffer(m_Device, m_SwapchainFramebuffers[i], nullptr);
         }

         if (m_Swapchain)
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
      }
   }


   void recreateSwapChain() {
      int width = 0, height = 0;
      while (width == 0 || height == 0) {
         glfwGetFramebufferSize(m_Window, &width, &height);
         glfwWaitEvents();
      }

      vkDeviceWaitIdle(m_Device);

      m_FramebufferResized = true;

      cleanupSwapchain();

      createSwapChain();
      createImageViews();
      createRenderPass();
      //createGraphicsPipeline();
      createFramebuffers();
      createUniformBuffers();
      createDescriptorPool();
      createDescriptorSets();
      createCommandBuffers();

      m_FramebufferResized = false;
   }


//      math::vec3 cameraPos(0.0f, 0.0f, 3.0f);
//      math::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
//      math::vec3 cameraDirection(math::normalize(cameraPos - cameraTarget));
//      math::vec3 up(0.0f, 1.0f, 0.0f);
//      math::vec3 cameraRight(math::normalize(math::cross(up, cameraDirection)));
//      math::vec3 cameraUp(math::cross(cameraDirection, cameraRight));

   void updateUniformBuffer(uint32_t currentImage) {
      static auto startTime = TIME_NOW;

      auto currentTime = TIME_NOW;
      float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

      UniformBufferObject ubo = {};
      ubo.model = math::rotate(math::mat4(1.0f), time * math::radians(90.0f), math::vec3(0.0f, 0.0f, 1.0f));

      math::vec3 cameraPos(2.0f, 2.0f, 2.0f);
      math::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
      math::vec3 upVector(0.0f, 0.0f, 1.0f);
      ubo.view = math::lookAt(cameraPos, cameraTarget, upVector);

      float verticalFov = math::radians(45.0f);
      float aspectRatio = m_SwapchainExtent.width / (float) m_SwapchainExtent.height;
      float nearPlane = 0.1f;
      float farPlane = 10.0f;
      ubo.proj = math::perspective(verticalFov, aspectRatio, nearPlane, farPlane);
      ubo.proj[1][1] *= -1;

      void* data;
      vkMapMemory(m_Device, m_UniformMemory[currentImage], 0, sizeof(ubo), 0, &data);
      memcpy(data, &ubo, sizeof(ubo));
      vkUnmapMemory(m_Device, m_UniformMemory[currentImage]);
   }


   void drawFrame() {
      vkWaitForFences(m_Device, 1, &m_Fences[m_FrameIndex], VK_TRUE, UINT64_MAX);

      uint32_t imageIndex;
      VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX,
                                              m_AcquireSemaphores[m_FrameIndex], nullptr, &imageIndex);

      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
         recreateSwapChain();
         return;
      }
      else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
         throw std::runtime_error("failed to acquire swap chain image!");
      }

      updateUniformBuffer(imageIndex);

      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = &m_AcquireSemaphores[m_FrameIndex];
      submitInfo.pWaitDstStageMask = waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &m_CmdBuffers[imageIndex];
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = &m_ReleaseSemaphores[m_FrameIndex];


      vkResetFences(m_Device, 1, &m_Fences[m_FrameIndex]);

      if (vkQueueSubmit(m_Queues[QUEUE_GRAPHICS], 1, &submitInfo, m_Fences[m_FrameIndex]) != VK_SUCCESS) {
         throw std::runtime_error("failed to submit draw command buffer!");
      }

      VkPresentInfoKHR presentInfo = {};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &m_ReleaseSemaphores[m_FrameIndex];
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &m_Swapchain;
      presentInfo.pImageIndices = &imageIndex;

      result = vkQueuePresentKHR(m_Queues[QUEUE_PRESENT], &presentInfo);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
         recreateSwapChain();
      }
      else if (result != VK_SUCCESS) {
         throw std::runtime_error("failed to present swap chain image!");
      }

      m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
   }


   void mainLoop() {
      std::chrono::milliseconds frameTiming(33);
      while (!glfwWindowShouldClose(m_Window)) {
         auto drawStart = TIME_NOW;

         glfwPollEvents();
         drawFrame();

         auto drawEnd = TIME_NOW;
         auto drawTime = std::chrono::duration_cast<std::chrono::milliseconds>(drawEnd - drawStart);
         if (drawTime < frameTiming) {
            std::this_thread::sleep_for(frameTiming - drawTime);
         }

         //auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_NOW - drawStart);
         //std::cout << "It took me " << frameTime.count() << " milliseconds." << std::endl;
      }

      vkDeviceWaitIdle(m_Device);
   }
};


int main() {
   Renderer app;

   //math::vec4 pos(2, 0, 0, 1);
   //math::mat4 mat = math::rotate(math::mat4(1.0f), math::radians(180.0f), math::vec3(1.0f, 1.0f, 0.0f));
   //math::vec4 newPos(pos * mat);
//   float test = math::radians(90);
//   test = math::radians(180);
//   math::vec3 tmp(3.0f, 2.0f, 1.0f);
//   test = math::magnitude(tmp);
//   math::vec3 multiplied(tmp * 3);
//   test = math::dot(tmp, tmp);

//   math::mat4 a({1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f},
//                {9.0f, 10.0f, 11.0f, 12.0f}, {13.0f, 14.0f, 15.0f, 16.0f});
//   math::mat4 b({16.0f, 15.0f, 14.0f, 13.0f}, {12.0f, 11.0f, 10.0f, 9.0f},
//                {8.0f, 7.0f, 6.0f, 5.0f}, {4.0f, 3.0f, 2.0f, 1.0f});
//   math::mat4 result(a * b);
//   result = b * a;
//
//   math::mat4 view(math::lookAt(math::vec3(0.0f, 0.0f, 3.0f),
//                                math::vec3(0.0f, 0.0f, 0.0f),
//                                math::vec3(0.0f, 1.0f, 0.0f)));

   //math::mat4 proj(math::perspective(math::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 10.0f));


   try {
      app.run();
   } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

