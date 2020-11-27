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


    void ShaderModule::ExtractIO(const spirv_cross::CompilerGLSL &compiler,
                                 const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
                                 std::vector<VertexBinding> &output) {

        output.emplace_back();
        output.back().binding = 0;  /// TODO: maybe later support multiple bindings if we use per_instance input rate?
        output.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        auto &vertexLayout = output.back().vertexLayout;

        for (const auto &resource : resources) {
            const auto &base_type = compiler.get_type(resource.base_type_id);
            uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            if (location >= vertexLayout.size()) vertexLayout.resize(location + 1);

            VertexAttribute attribute{};
            switch (base_type.basetype) {
                case spirv_cross::SPIRType::BaseType::Float: {
                    switch (base_type.width) {
                        case 16: {
                            switch (base_type.vecsize) {
                                case 1:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R16_SFLOAT, 2};
                                    break;
                                case 2:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R16G16_SFLOAT, 4};
                                    break;
                                case 3:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R16G16B16_SFLOAT, 6};
                                    break;
                                case 4:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R16G16B16A16_SFLOAT, 8};
                                    break;
                                default:
                                    throw std::runtime_error("[ShaderModule] Unsupported vector size");
                            }
                            break;
                        }
                        case 32: {
                            switch (base_type.vecsize) {
                                case 1:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R32_SFLOAT, 4};
                                    break;
                                case 2:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R32G32_SFLOAT, 8};
                                    break;
                                case 3:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R32G32B32_SFLOAT, 12};
                                    break;
                                case 4:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R32G32B32A32_SFLOAT, 16};
                                    break;
                                default:
                                    throw std::runtime_error("[ShaderModule] Unsupported vector size");
                            }
                            break;
                        }
                    }
                    break;
                }
                case spirv_cross::SPIRType::BaseType::Double: {
                    switch (base_type.width) {
                        case 64: {
                            switch (base_type.vecsize) {
                                case 1:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R64_SFLOAT, 8};
                                    break;
                                case 2:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R64G64_SFLOAT, 16};
                                    break;
                                case 3:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R64G64B64_SFLOAT, 24};
                                    break;
                                case 4:
                                    attribute = VertexAttribute{resource.name, VK_FORMAT_R64G64B64A64_SFLOAT, 32};
                                    break;
                                default:
                                    throw std::runtime_error("[ShaderModule] Unsupported vector size");
                            }
                            break;
                        }
                    }
                    break;
                }
                default:
                    throw std::runtime_error("[ShaderModule] Unsupported shader stage input attribute type!");
            }
            vertexLayout[location] = attribute;
            std::cout << "[ShaderModule (" << m_Filepath << ")] Shader input: " << resource.name << std::endl;
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
            const auto& baseType = glsl.get_type(resource.base_type_id);
//            size_t memberCount = baseType.member_types.size();
//            uint32_t totalSize = 0;
//            for (uint32_t i = 0; i < memberCount; i++) {
//                auto memberSize = glsl.get_declared_struct_member_size(baseType, i);
//                totalSize += memberSize;
//            }
            auto size = glsl.get_declared_struct_size(baseType);
            uint32_t bindingKey = (setIdx << 16u) + bindingIdx;
            m_DescriptorSetLayouts[bindingKey] = DescriptorBinding{
                    resource.name,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
//                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    (uint32_t)size,
                    1
            };

            std::cout << "[ShaderModule (" << filename << ")] UBO: "
                      << resource.name << ", Set: " << setIdx << ", Binding: " << bindingIdx << std::endl;
        }

        /// Extract push constants
        for (auto& resource : resources.push_constant_buffers) {
            auto ranges = glsl.get_active_buffer_ranges(resource.id);
            for (auto &range : ranges) {
                m_PushRanges[range.index] = {
                        VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM,
                        (uint32_t)range.offset,
                        (uint32_t)range.range,
                };
            }
        }

        /// Extract sampled images
        for (auto &resource : resources.sampled_images) {
            DescriptorBinding binding{};

            const auto &type = glsl.get_type(resource.type_id);
            if (type.array.empty()) {
                binding.count = 1;
            } else {
                binding.count = type.pointer ? 0 : type.array[0];
            }

            switch (type.basetype) {
                case spirv_cross::SPIRType::BaseType::SampledImage:
                    binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;

                default:
                    throw std::runtime_error("[ShaderModule] Unsupported sampled image type!");
            }
            uint32_t setIdx = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t bindingIdx = glsl.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t bindingKey = (setIdx << 16u) + bindingIdx;
            m_DescriptorSetLayouts[bindingKey] = binding;

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


    void Image::ChangeLayout(const CommandBuffer &cmdBuffer, VkImageLayout newLayout) {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_CurrentLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_MipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkPipelineStageFlags destinationStage;
        switch (m_CurrentLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = 0;
                switch (newLayout) {
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        m_CurrentStage = VK_PIPELINE_STAGE_HOST_BIT;
                        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        if (hasStencilComponent(m_Format))
                            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        barrier.dstAccessMask =
                                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        break;

                    default:
                        throw std::invalid_argument("unsupported layout transition!");
                        break;
                }
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    //barrier.srcQueueFamilyIndex = DeviceQueueIndices[QUEUE_TRANSFER];
                    //barrier.dstQueueFamilyIndex = DeviceQueueIndices[QUEUE_GRAPHICS];

                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;

                    //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

                    //destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                } else {
                    throw std::invalid_argument("unsupported layout transition!");
                }
                break;

            default:
                throw std::invalid_argument("unsupported layout transition!");
                break;
        }

        vkCmdPipelineBarrier(cmdBuffer.data(), m_CurrentStage, destinationStage, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier
        );

        m_CurrentLayout = newLayout;
        m_CurrentStage = destinationStage;
    }


    void Image::ChangeLayout(const CommandBuffer &cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags dstStage,
                             VkImageAspectFlags aspectFlags) {
        /// TODO: this needs to be rewritten. I misunderstood some key aspects
        /// about memory synchronization when I started writing this.
        /// Resource to use: http://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
        VkImageMemoryBarrier barrier = s_BaseBarrier;
        barrier.oldLayout = m_CurrentLayout;
        barrier.newLayout = newLayout;
        barrier.image = m_Image;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.levelCount = m_MipLevels;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.layerCount = 1;

        switch (m_CurrentLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = 0;
                switch (newLayout) {
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        m_CurrentStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        if (hasStencilComponent(m_Format))
                            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        barrier.dstAccessMask =
                                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        break;

                    default:
                        throw std::invalid_argument("unsupported layout transition!");
                        break;
                }
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                    if (dstStage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
                        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    } else {
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                    }
                } else {
                    throw std::invalid_argument("unsupported layout transition!");
                }
                break;

            default:
                throw std::invalid_argument("unsupported layout transition!");
                break;
        }

        vkCmdPipelineBarrier(cmdBuffer.data(), m_CurrentStage, dstStage, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier
        );

        m_CurrentLayout = newLayout;
        m_CurrentStage = dstStage;
    }


    void Image::GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer &cmdBuffer) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, m_Format, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        if (m_CurrentLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            throw std::runtime_error(
                    "image must be in 'VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL' layout when generating mipmaps!");

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_Image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = Extent.width;
        int32_t mipHeight = Extent.height;
        for (uint32_t i = 1; i < m_MipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = m_CurrentLayout;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer.data(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmdBuffer.data(),
                           m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_MipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        std::array<VkImageMemoryBarrier, 2> barriers = {barrier, barrier};
        barriers[1].subresourceRange.baseMipLevel = m_MipLevels - 1;
        barriers[1].subresourceRange.levelCount = 1;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmdBuffer.data(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), barriers.data());

        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

        setupDebugMessenger();
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
                               VkMemoryPropertyFlags flags) : m_Device(device) {

        uint32_t resultTypeBits = ~(0U);
        uint32_t totalSize = 0;
        for (const auto *image : images) {
            resultTypeBits &= image->MemoryInfo().memoryTypeBits;
            totalSize += roundUp(image->MemoryInfo().size, image->MemoryInfo().alignment);
        }

        Allocate(findMemoryType(physDevice, resultTypeBits, flags), totalSize);
    }
}
