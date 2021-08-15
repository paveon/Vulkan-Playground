#define GLFW_INCLUDE_VULKAN

#include "vulkan_wrappers.h"
#include "utils.h"
#include "spirv_cross.hpp"

#include <GLFW/glfw3.h>
#include <unordered_map>

#include <iostream>
#include <spirv_glsl.hpp>


namespace vk {
    const std::array<const char *, 2> LogicalDevice::s_RequiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };

    LogicalDevice::LogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) :
            m_Physical(physicalDevice), m_Surface(surface) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Physical, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Physical, &queueFamilyCount,
                                                 queueFamilies.data());

        m_QueueIndices.resize(static_cast<size_t>(QueueFamily::COUNT));

        std::set<uint32_t> uniqueQueueFamilies;
        bool graphicsFamilyExists = false;
        bool presentFamilyExists = false;
        bool transferFamilyExists = false;
        bool computeFamilyExists = false;
        int fallbackTransferQueue = -1;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueCount > 0) {
                auto flags = queueFamilies[i].queueFlags;
                if (!graphicsFamilyExists && flags & VK_QUEUE_GRAPHICS_BIT) {
                    m_QueueIndices[static_cast<size_t>(QueueFamily::GRAPHICS)] = i;
                    uniqueQueueFamilies.insert(i);
                    graphicsFamilyExists = true;
                }

                if (!computeFamilyExists && flags & VK_QUEUE_COMPUTE_BIT) {
                    m_QueueIndices[static_cast<size_t>(QueueFamily::COMPUTE)] = i;
                    uniqueQueueFamilies.insert(i);
                    computeFamilyExists = true;
                }

                if (!transferFamilyExists && flags & VK_QUEUE_TRANSFER_BIT) {
                    if (!(flags & VK_QUEUE_GRAPHICS_BIT)) {
                        m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)] = i;
                        uniqueQueueFamilies.insert(i);
                        transferFamilyExists = true;
                    } else {
                        fallbackTransferQueue = i;
                    }
                }

                if (!presentFamilyExists) {
                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(m_Physical, i, m_Surface, &presentSupport);
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
        } else if (!presentFamilyExists) {
            throw std::runtime_error("GPU does not support present command queue!");
        } else if (!transferFamilyExists) {
            if (fallbackTransferQueue >= 0) {
                m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)] = fallbackTransferQueue;
                uniqueQueueFamilies.insert(fallbackTransferQueue);
            } else throw std::runtime_error("GPU does not support transfer command queue!");
        }


        /* Verify that device supports all required extensions */
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_Physical, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_Physical, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(s_RequiredExtensions.begin(), s_RequiredExtensions.end());
        for (const auto &extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        if (!requiredExtensions.empty()) {
            for (const auto &missing : requiredExtensions) {
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
        deviceFeatures.geometryShader = VK_TRUE;

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
        physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = s_RequiredExtensions.size();
        createInfo.ppEnabledExtensionNames = s_RequiredExtensions.data();
        createInfo.pNext = &physicalDeviceDescriptorIndexingFeatures;

        if (vkCreateDevice(m_Physical, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        m_Queues.resize(m_QueueIndices.size());
        for (size_t i = 0; i < m_QueueIndices.size(); i++)
            vkGetDeviceQueue(m_Device, m_QueueIndices[i], 0, &m_Queues[i]);
    }


    auto LogicalDevice::getSurfaceFormats() const -> std::vector<VkSurfaceFormatKHR> {
        uint32_t formatCount = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_Physical, m_Surface, &formatCount, nullptr) !=
            VK_SUCCESS)
            throw std::runtime_error("Could not query surface for supported formats");
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_Physical, m_Surface, &formatCount, formats.data()) !=
            VK_SUCCESS)
            throw std::runtime_error("Could not query surface for supported formats");
        return formats;
    }


    auto LogicalDevice::getSurfacePresentModes() const -> std::vector<VkPresentModeKHR> {
        uint32_t presentModeCount = 0;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_Physical, m_Surface, &presentModeCount,
                                                      nullptr) !=
            VK_SUCCESS)
            throw std::runtime_error("Could not query surface for supported present modes");
        std::vector<VkPresentModeKHR> modes(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_Physical, m_Surface, &presentModeCount,
                                                      modes.data()) != VK_SUCCESS)
            throw std::runtime_error("Could not query surface for supported present modes");
        return modes;
    }


    auto ShaderModule::ParseSpirVType(const spirv_cross::SPIRType &type) -> std::pair<VkFormat, uint32_t> {
        switch (type.basetype) {
            case spirv_cross::SPIRType::BaseType::Float: {
                switch (type.width) {
                    case 16: {
                        switch (type.vecsize) {
                            case 1:
                                return {VK_FORMAT_R16_SFLOAT, 2 * type.columns};
                            case 2:
                                return {VK_FORMAT_R16G16_SFLOAT, 4 * type.columns};
                            case 3:
                                return {VK_FORMAT_R16G16B16_SFLOAT, 6 * type.columns};
                            case 4:
                                return {VK_FORMAT_R16G16B16A16_SFLOAT, 8 * type.columns};
                            default:
                                throw std::runtime_error("[ShaderModule] Unsupported vector size");
                        }
                    }
                    case 32: {
                        switch (type.vecsize) {
                            case 1:
                                return {VK_FORMAT_R32_SFLOAT, 4 * type.columns};
                            case 2:
                                return {VK_FORMAT_R32G32_SFLOAT, 8 * type.columns};
                            case 3:
                                return {VK_FORMAT_R32G32B32_SFLOAT, 12 * type.columns};
                            case 4:
                                return {VK_FORMAT_R32G32B32A32_SFLOAT, 16 * type.columns};
                            default:
                                throw std::runtime_error("[ShaderModule] Unsupported vector size");
                        }
                    }
                    default:
                        throw std::runtime_error("[ShaderModule] Unsupported vector component size");
                }
            }
            case spirv_cross::SPIRType::BaseType::Double: {
                switch (type.width) {
                    case 64: {
                        switch (type.vecsize) {
                            case 1:
                                return {VK_FORMAT_R64_SFLOAT, 8 * type.columns};
                            case 2:
                                return {VK_FORMAT_R64G64_SFLOAT, 16 * type.columns};
                            case 3:
                                return {VK_FORMAT_R64G64B64_SFLOAT, 24 * type.columns};
                            case 4:
                                return {VK_FORMAT_R64G64B64A64_SFLOAT, 32 * type.columns};
                            default:
                                throw std::runtime_error("[ShaderModule] Unsupported vector size");
                        }
                    }
                    default:
                        throw std::runtime_error("[ShaderModule] Unsupported vector component size");
                }
            }
            case spirv_cross::SPIRType::BaseType::Int: {
                switch (type.width) {
                    case 32: {
                        switch (type.vecsize) {
                            case 1:
                                return {VK_FORMAT_R32_SINT, 4 * type.columns};
                            case 2:
                                return {VK_FORMAT_R32G32_SINT, 8 * type.columns};
                            case 3:
                                return {VK_FORMAT_R32G32B32_SINT, 12 * type.columns};
                            case 4:
                                return {VK_FORMAT_R32G32B32A32_SINT, 16 * type.columns};
                            default:
                                throw std::runtime_error("[ShaderModule] Unsupported vector size");
                        }
                    }
                    default:
                        throw std::runtime_error("[ShaderModule] Unsupported vector component size");
                }
            }
            case spirv_cross::SPIRType::BaseType::UInt: {
                switch (type.width) {
                    case 32: {
                        switch (type.vecsize) {
                            case 1:
                                return {VK_FORMAT_R32_UINT, 4 * type.columns};
                            case 2:
                                return {VK_FORMAT_R32G32_UINT, 8 * type.columns};
                            case 3:
                                return {VK_FORMAT_R32G32B32_UINT, 12 * type.columns};
                            case 4:
                                return {VK_FORMAT_R32G32B32A32_UINT, 16 * type.columns};
                            default:
                                throw std::runtime_error("[ShaderModule] Unsupported vector size");
                        }
                    }
                    default:
                        throw std::runtime_error("[ShaderModule] Unsupported vector component size");
                }
            }
            default:
                throw std::runtime_error("[ShaderModule] Unsupported shader SPIRV data type!");
        }
        return {VK_FORMAT_UNDEFINED, 0};
    }


    void ShaderModule::ExtractIO(const spirv_cross::CompilerGLSL &compiler,
                                 const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
                                 std::vector<VertexBinding> &output) {

        VertexBinding binding{
                {},
                0,    /// TODO: maybe later support multiple bindings if we use per_instance input rate?
                VK_VERTEX_INPUT_RATE_VERTEX,
        };
        auto &vertexLayout = binding.vertexLayout;

        for (const auto &resource : resources) {
            const auto &base_type = compiler.get_type(resource.base_type_id);
            uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            if (location >= vertexLayout.size()) vertexLayout.resize(location + 1);

            auto[vkFormat, size] = ParseSpirVType(base_type);
            VertexAttribute attribute{resource.name, vkFormat, size};
            vertexLayout[location] = attribute;
            std::cout << "[ShaderModule (" << m_Filepath << ")] Shader input: " << resource.name << std::endl;
        }

        if (!vertexLayout.empty()) {
            output.push_back(binding);
        }
    }


    ShaderModule::ShaderModule(VkDevice device, const std::string &filename) : m_Filepath(filename), m_Device(device) {
        std::vector<uint32_t> shaderCode = readFile(filename);
        spirv_cross::CompilerGLSL glsl(shaderCode);
        spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        ExtractIO(glsl, resources.stage_inputs, m_VertexInputBindings);
        ExtractIO(glsl, resources.stage_outputs, m_VertexOutputBindings);

        /// Extract uniform buffers
        for (auto &resource : resources.uniform_buffers) {
            uint32_t setIdx = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t bindingIdx = glsl.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t bindingKey = (setIdx << 16u) + bindingIdx;
            const auto &baseType = glsl.get_type(resource.base_type_id);
            uint32_t size = glsl.get_declared_struct_size(baseType);

            std::unordered_map<std::string, UniformMember> members;
            size_t memberCount = baseType.member_types.size();
            for (uint32_t i = 0; i < memberCount; i++) {
                auto memberTypeID = baseType.member_types[i];
                const auto &memberType = glsl.get_type(memberTypeID);
//                auto[vkFormat, typeSize] = ParseSpirVType(memberType);
                const auto &memberName = glsl.get_member_name(resource.base_type_id, i);
                uint32_t memberSize = glsl.get_declared_struct_member_size(baseType, i);
                uint32_t offset = glsl.type_struct_member_offset(baseType, i);
//                assert(memberSize == typeSize);
                members[memberName] = {offset, memberSize};
            }

            m_UniformBindings.emplace(bindingKey, UniformBinding{
                    resource.name,
                    size,
                    1,
                    false,
                    std::move(members)
            });

            std::cout << "[ShaderModule (" << filename << ")] UBO: "
                      << resource.name << ", Set: " << setIdx << ", Binding: " << bindingIdx << std::endl;
        }

        /// Extract push constants
        for (auto &resource : resources.push_constant_buffers) {
            auto ranges = glsl.get_active_buffer_ranges(resource.id);
            for (auto &range : ranges) {
                m_PushRanges[range.index] = {
                        VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM,
                        (uint32_t) range.offset,
                        (uint32_t) range.range,
                };
            }
        }

        /// Extract sampled images
        for (auto &resource : resources.sampled_images) {
            SamplerBinding binding{};
            binding.name = resource.name;
            const auto &type = glsl.get_type(resource.type_id);
            if (type.array.empty()) {
                binding.count = 1;
            } else {
                binding.count = type.pointer ? 0 : type.array[0];
                binding.array = true;
            }

            switch (type.image.dim) {
                case spv::Dim::Dim1D:
                    binding.type = SamplerBinding::Type::SAMPLER_1D;
                    break;
                case spv::Dim::Dim2D:
                    binding.type = SamplerBinding::Type::SAMPLER_2D;
                    break;
                case spv::Dim::Dim3D:
                    binding.type = SamplerBinding::Type::SAMPLER_3D;
                    break;
                case spv::Dim::DimCube:
                    binding.type = SamplerBinding::Type::SAMPLER_CUBE;
                    break;
                default:
                    throw std::runtime_error("[ShaderModule] Unsupported sampler dimension type");
                    break;
            }

            switch (type.basetype) {
                case spirv_cross::SPIRType::BaseType::SampledImage:
//                    binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;

                default:
                    throw std::runtime_error("[ShaderModule] Unsupported sampled image type!");
            }
            uint32_t setIdx = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t bindingIdx = glsl.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t bindingKey = (setIdx << 16u) + bindingIdx;
            m_SamplerBindings.emplace(bindingKey, binding);

            std::cout << "[ShaderModule (" << filename << ")] Sampler: "
                      << resource.name << ", Set: " << setIdx << ", Binding: " << bindingIdx << std::endl;
        }

        m_CodeSize = shaderCode.size() * sizeof(uint32_t);
        std::cout << "[Vulkan] Loaded shader '" << filename << "' bytecode, bytes: " << m_CodeSize << std::endl;

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = m_CodeSize;
        createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.data());

        if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");
    }


    void Image::ChangeLayout(const CommandBuffer &cmdBuffer,
                             VkImageLayout srcLayout,
                             VkImageLayout dstLayout,
                             VkPipelineStageFlags srcStages,
                             VkPipelineStageFlags dstStages,
                             VkAccessFlags srcMask,
                             VkAccessFlags dstMask,
                             const std::vector<VkImageSubresourceRange> &subresources) {

        std::vector<VkImageMemoryBarrier> barriers;
        if (subresources.empty()) {
            barriers.push_back(GetBaseBarrier(srcLayout, dstLayout, srcMask, dstMask));
            barriers[0].subresourceRange.baseMipLevel = 0;
            barriers[0].subresourceRange.levelCount = m_Info.mipLevels;
            barriers[0].subresourceRange.baseArrayLayer = 0;
            barriers[0].subresourceRange.layerCount = m_Info.arrayLayers;
        } else {
            barriers = std::vector<VkImageMemoryBarrier>(
                    subresources.size(),
                    GetBaseBarrier(srcLayout, dstLayout, srcMask, dstMask)
            );
            for (size_t i = 0; i < subresources.size(); i++) {
                barriers[i].subresourceRange = subresources[i];
            }
        }

        vkCmdPipelineBarrier(cmdBuffer.data(), srcStages, dstStages, 0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), barriers.data()
        );
    }


    void Image::GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer &cmdBuffer) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, m_Info.format, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_Image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        for (uint32_t layer = 0; layer < m_Info.arrayLayers; layer++) {
            barrier.subresourceRange.baseArrayLayer = layer;
            int32_t mipWidth = m_Info.extent.width;
            int32_t mipHeight = m_Info.extent.height;

            for (uint32_t mipLevel = 1; mipLevel < m_Info.mipLevels; mipLevel++) {
                if (mipLevel > 1) {
                    /// Wait for previous level to be filled with blit command,
                    /// base level 0 must be prepared externally before calling this function
                    barrier.subresourceRange.baseMipLevel = mipLevel - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(cmdBuffer.data(),
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         0,
                                         0, nullptr,
                                         0, nullptr,
                                         1, &barrier);
                }

                VkImageBlit blit = {};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = mipLevel - 1;
                blit.srcSubresource.baseArrayLayer = layer;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = mipLevel;
                blit.dstSubresource.baseArrayLayer = layer;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(cmdBuffer.data(),
                               m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit,
                               VK_FILTER_LINEAR);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.layerCount = m_Info.arrayLayers;
        barrier.subresourceRange.levelCount = m_Info.mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        std::array<VkImageMemoryBarrier, 2> barriers = {barrier, barrier};
        barriers[1].subresourceRange.baseMipLevel = m_Info.mipLevels - 1;
        barriers[1].subresourceRange.levelCount = 1;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmdBuffer.data(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), barriers.data());
    }


    Swapchain::Swapchain(
            VkDevice device,
            const std::set<uint32_t> &queueIndices,
            VkExtent2D extent,
            VkSurfaceKHR surface,
            const VkSurfaceCapabilitiesKHR &capabilities,
            const VkSurfaceFormatKHR &surfaceFormat, VkPresentModeKHR presentMode) :
            m_Device(device),
            m_Capabilities(capabilities),
            m_Extent(extent) {

        uint32_t imageCount = m_Capabilities.minImageCount + 1;
        if (m_Capabilities.maxImageCount > 0 && imageCount > m_Capabilities.maxImageCount)
            imageCount = m_Capabilities.maxImageCount;

        m_Format = surfaceFormat.format;

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = m_Format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = m_Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        createInfo.preTransform = m_Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = nullptr;

        if (queueIndices.size() < 2) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        } else {
            std::vector<uint32_t> uniqueIndices(queueIndices.begin(), queueIndices.end());
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = uniqueIndices.size();
            createInfo.pQueueFamilyIndices = uniqueIndices.data();
        }

        if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
            throw std::runtime_error("Swapchain construction failed");

        if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Swapchain image query failed");
        m_Images.resize(imageCount);
        if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data()) != VK_SUCCESS)
            throw std::runtime_error("Swapchain image query failed");


        for (VkImage image : m_Images)
            m_ImageViews.emplace_back(ImageView(m_Device, image, m_Format, 1, VK_IMAGE_ASPECT_COLOR_BIT));
    }


    Instance::Instance(bool enableValidation) {
        if (enableValidation && !checkValidationLayerSupport(ArrayToVector(s_ValidationLayers))) {
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
        auto extensions = getRequiredVulkanExtensions(enableValidation);
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidation) {
            createInfo.enabledLayerCount = s_ValidationLayers.size();
            createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
            debugCreateInfo = createDebugMsgInfo(debugCallback, nullptr);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
            throw std::runtime_error("Vulkan instance could not be created!");

        if (enableValidation) {
            setupDebugMessenger();
        }
    }


    void Instance::Release() noexcept {
        if (m_Instance) {
            if (m_DebugMessenger) DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            vkDestroyInstance(m_Instance, nullptr);
        }
    }


    void Instance::setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = createDebugMsgInfo(debugCallback, nullptr);
        if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
            throw std::runtime_error("Could not create Vulkan debug messenger");
    }


    Surface::Surface(VkInstance instance, GLFWwindow *window) : m_Instance(instance) {
        if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    DeviceMemory::DeviceMemory(VkPhysicalDevice physDevice,
                               VkDevice device,
                               const std::vector<const Image *> &images,
                               std::optional<VkDeviceSize> sizeOverride,
                               VkMemoryPropertyFlags flags) : m_Device(device) {

        uint32_t resultTypeBits = ~(0U);
        VkDeviceSize totalSize = 0;
        for (const auto *image : images) {
            resultTypeBits &= image->MemoryInfo().memoryTypeBits;
            totalSize += math::roundUp(image->MemoryInfo().size, image->MemoryInfo().alignment);
        }

        if (sizeOverride.has_value()) {
            totalSize = std::max(totalSize, sizeOverride.value());
        }

        Allocate(findMemoryType(physDevice, resultTypeBits, flags), totalSize);
    }


    DeviceMemory::DeviceMemory(VkPhysicalDevice physDevice,
                               VkDevice device,
                               const std::vector<const Buffer *> &buffers,
                               VkMemoryPropertyFlags flags) : m_Device(device) {

        uint32_t resultTypeBits = ~(0U);
        uint32_t totalSize = 0;
        for (const auto *buffer : buffers) {
            const auto &info = buffer->MemoryInfo();
            resultTypeBits &= info.memoryTypeBits;
            totalSize += math::roundUp(info.size, info.alignment);
        }

        Allocate(findMemoryType(physDevice, resultTypeBits, flags), totalSize);
    }


    DescriptorSetLayout::DescriptorSetLayout(VkDevice device,
                                             const std::vector<VkDescriptorSetLayoutBinding> &bindings)
            : m_Device(device) {

        std::vector<VkDescriptorBindingFlagsEXT> bindingFlags(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++) {
            if (bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;
            }
        }
        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT ext{};
        ext.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        ext.bindingCount = bindingFlags.size();
        ext.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();
        layoutInfo.pNext = &ext;

        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }


    Sampler::Sampler(VkDevice device, float maxLod) : m_Device(device) {
        VkSamplerCreateInfo samplerInfo = {
                VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                nullptr,
                0,
                VK_FILTER_LINEAR,
                VK_FILTER_LINEAR,
                VK_SAMPLER_MIPMAP_MODE_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                0,
                VK_TRUE,
                16,
                VK_FALSE,
                VK_COMPARE_OP_ALWAYS,
                0.0f,
                maxLod,
                VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                VK_FALSE
        };

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture sampler!");
    }
}
