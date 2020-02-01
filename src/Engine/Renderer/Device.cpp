#include "Device.h"

#include <iostream>
#include <set>

using namespace vk;

const std::array<const char*, 1> Device::s_RequiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

Device::Device(const vk::Instance& instance, const vk::Surface& surface) : m_Instance(&instance), m_Surface(&surface) {
   m_PhysicalDevice = pickPhysicalDevice();
   createLogicalDevice();
   m_CommandPool = vk::CommandPool(m_LogicalDevice, GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}


VkPhysicalDevice Device::pickPhysicalDevice() {
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(m_Instance->data(), &deviceCount, nullptr);
   if (deviceCount == 0) { throw std::runtime_error("No GPU with Vulkan support!"); }
   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(m_Instance->data(), &deviceCount, devices.data());

   VkPhysicalDevice fallback = nullptr;
   VkPhysicalDeviceFeatures supportedFeatures;
   for (const auto& device : devices) {
      vkGetPhysicalDeviceProperties(device, &m_Properties);
      vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

      if (supportedFeatures.samplerAnisotropy) {
         if (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            std::cout << "[Vulkan] Picking discrete GPU: " << m_Properties.deviceName << std::endl;
            VkSampleCountFlags counts = std::min(m_Properties.limits.framebufferColorSampleCounts,
                                                 m_Properties.limits.framebufferDepthSampleCounts);

            if (counts & VK_SAMPLE_COUNT_64_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_64_BIT; }
            else if (counts & VK_SAMPLE_COUNT_32_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_32_BIT; }
            else if (counts & VK_SAMPLE_COUNT_16_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_16_BIT; }
            else if (counts & VK_SAMPLE_COUNT_8_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_8_BIT; }
            else if (counts & VK_SAMPLE_COUNT_4_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_4_BIT; }
            else if (counts & VK_SAMPLE_COUNT_2_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_2_BIT; }
            return device;
         }
         else if (!fallback) {
            fallback = device;
         }
      }
   }

   if (!fallback) throw std::runtime_error("No suitable GPU!");

   vkGetPhysicalDeviceProperties(fallback, &m_Properties);
   std::cout << "[Vulkan] Picking fallback GPU: " << m_Properties.deviceName << std::endl;
   return fallback;
}


void Device::createLogicalDevice() {
   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount,
                                            queueFamilies.data());

   m_QueueIndices.resize(static_cast<size_t>(QueueFamily::COUNT));

   std::set<uint32_t> uniqueQueueFamilies;
   bool graphicsFamilyExists = false;
   bool presentFamilyExists = false;
   bool transferFamilyExists = false;
   int fallbackTransferQueue = -1;
   for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueCount > 0) {
         auto flags = queueFamilies[i].queueFlags;
         if (!graphicsFamilyExists && flags & VK_QUEUE_GRAPHICS_BIT) {
            m_QueueIndices[static_cast<size_t>(QueueFamily::GRAPHICS)] = i;
            uniqueQueueFamilies.insert(i);
            graphicsFamilyExists = true;
         }

         if (!transferFamilyExists && flags & VK_QUEUE_TRANSFER_BIT) {
            if (!(flags & VK_QUEUE_GRAPHICS_BIT)) {
               m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)] = i;
               uniqueQueueFamilies.insert(i);
               transferFamilyExists = true;
            }
            else {
               fallbackTransferQueue = i;
            }
         }

         if (!presentFamilyExists) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface->data(), &presentSupport);
            if (presentSupport) {
               m_QueueIndices[static_cast<size_t>(QueueFamily::PRESENT)] = i;
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
      if (fallbackTransferQueue >= 0) {
         m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)] = fallbackTransferQueue;
         uniqueQueueFamilies.insert(fallbackTransferQueue);
      }
      else throw std::runtime_error("GPU does not support transfer command queue!");
   }


   /* Verify that device supports all required extensions */
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

   std::set<std::string> requiredExtensions(s_RequiredExtensions.begin(), s_RequiredExtensions.end());
   for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
   }
   if (!requiredExtensions.empty()) {
      for (const auto& missing : requiredExtensions) {
         std::cerr << "[Vulkan] Missing extension: " << missing << std::endl;
      }
      throw std::runtime_error("GPU does not support required extensions!");
   }


   /* Verify that surface supports at least some formats and present modes */
   if (getSurfaceFormats().empty() || getSurfacePresentModes().empty()) {
      throw std::runtime_error("GPU does not support required surface features!");
   }

   /* Create device with specified queues */
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
   deviceFeatures.samplerAnisotropy = VK_TRUE;
   deviceFeatures.sampleRateShading = VK_TRUE;

   VkDeviceCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.queueCreateInfoCount = queueCreateInfos.size();
   createInfo.pQueueCreateInfos = queueCreateInfos.data();
   createInfo.pEnabledFeatures = &deviceFeatures;
   createInfo.enabledExtensionCount = s_RequiredExtensions.size();
   createInfo.ppEnabledExtensionNames = s_RequiredExtensions.data();

   if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device!");

   m_Queues.resize(m_QueueIndices.size());
   for (size_t i = 0; i < m_QueueIndices.size(); i++)
      vkGetDeviceQueue(m_LogicalDevice, m_QueueIndices[i], 0, &m_Queues[i]);
}


uint32_t Device::findMemoryType(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties) const {
   VkPhysicalDeviceMemoryProperties memProperties;
   vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((requirements.memoryTypeBits & (1UL << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   throw std::runtime_error("failed to find suitable memory type!");
}


Swapchain Device::createSwapChain(const std::set<uint32_t>& queueIndices, VkExtent2D extent) const {
   VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities();
   VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(getSurfaceFormats());
   VkPresentModeKHR presentMode = chooseSwapPresentMode(getSurfacePresentModes());
   return Swapchain(m_LogicalDevice, queueIndices, extent, m_Surface->data(), capabilities, surfaceFormat, presentMode);
}


VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                     VkFormatFeatureFlags features) const {
   for (VkFormat format : candidates) {
      VkFormatProperties properties;
      vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &properties);

      switch (tiling) {
         case VK_IMAGE_TILING_LINEAR:
            if ((properties.linearTilingFeatures & features) == features) return format;
            break;

         case VK_IMAGE_TILING_OPTIMAL:
            if ((properties.optimalTilingFeatures & features) == features) return format;
            break;

         default:
            continue;
      }
   }

   throw std::runtime_error("failed to find desirable supported format!");
}