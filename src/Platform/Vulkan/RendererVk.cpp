#define GLFW_INCLUDE_VULKAN

//#define MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>
#include <Engine/Application.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Camera.h>
#include <backends/imgui_impl_vulkan.h>
#include <Engine/Core.h>

#include "RendererVk.h"
#include "ShaderPipelineVk.h"
#include "RenderPassVk.h"
#include "TextureVk.h"


const std::map<ShaderType, const char *> RendererVk::POST_PROCESS_SHADERS{
        {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/fullscreenQuad.vert.spv"},
        {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/postprocess.frag.spv"}
};

const std::map<ShaderType, const char *> RendererVk::NORMALDEBUG_SHADERS{
        {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/normaldebug.vert.spv"},
        {ShaderType::GEOMETRY_SHADER, BASE_DIR "/shaders/normaldebug.geom.spv"},
        {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/normaldebug.frag.spv"}
};

const std::map<ShaderType, const char *> RendererVk::SKYBOX_SHADERS{
        {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/skybox.vert.spv"},
        {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/skybox.frag.spv"}
};

uint32_t RendererVk::s_SkyboxTexIdx = 0;


void RendererVk::InitializeStaticResources() {
   InitializeTextureResources();
}

void RendererVk::ReleaseStaticResources() {
   ReleaseTextureResources();
}


RendererVk::RendererVk() :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_StageBuffer(&m_Device, 100'000'000),
        m_MeshDeviceBuffer(&m_Device, 100'000'000,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        m_UniformBuffer(&m_Device, 10'000'000) {


    VkPhysicalDeviceMemoryProperties memProperties;
    VkMemoryHeap *heaps = memProperties.memoryHeaps;
    vkGetPhysicalDeviceMemoryProperties(m_Device, &memProperties);

    std::vector<std::pair<uint32_t, VkMemoryType>> deviceLocalTypes;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        VkMemoryPropertyFlags flags = memProperties.memoryTypes[i].propertyFlags;
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            deviceLocalTypes.emplace_back(i, memProperties.memoryTypes[i]);
        }
    }

    std::sort(deviceLocalTypes.begin(), deviceLocalTypes.end(), [heaps](const auto &x, const auto &y) {
        return heaps[x.second.heapIndex].size > heaps[y.second.heapIndex].size;
    });

    assert(!deviceLocalTypes.empty());

    uint32_t memoryTypeIndex = deviceLocalTypes[0].first;
    m_MemoryIndices[vk::DeviceMemory::UsageType::DEVICE_LOCAL_LARGE] = memoryTypeIndex;

    VkDeviceSize memorySize = 512'000'000;
    m_ImageMemory = vk::DeviceMemory(m_Device, memoryTypeIndex, memorySize);

    vk::Swapchain &swapchain(m_Context.Swapchain());
    const auto &swapchainViews = swapchain.ImageViews();
    auto extent = swapchain.Extent();
    auto maxImgCount = swapchain.Capabilities().maxImageCount;

    m_SwapchainImageCount = swapchainViews.size();
    m_RenderPass = std::make_unique<RenderPassVk>(true);
    m_PostprocessRenderPass = std::make_unique<RenderPassVk>(false);

//    VkClearColorValue clearColorValue{{0.0f, 0.0f, 0.0f, 1.0f}};

    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};
    m_RenderPass->SetClearValue(1, clearValue);

    MultisampleState msState{};
    msState.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    DepthState depthState{};
    depthState.testEnable = VK_FALSE;
    depthState.writeEnable = VK_FALSE;
    depthState.boundTestEnable = VK_FALSE;
    depthState.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthState.min = 0.0f;
    depthState.max = 1.0f;
    m_PostprocessPipeline = std::make_unique<ShaderPipelineVk>(
            std::string("Post Process Pipeline"),
            POST_PROCESS_SHADERS,
            std::unordered_set<BindingKey>{},
            *m_PostprocessRenderPass,
            0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
            depthState,
            msState);

    depthState.testEnable = VK_TRUE;
    depthState.writeEnable = VK_TRUE;
    depthState.compareOp = VK_COMPARE_OP_LESS;
    msState.sampleCount = m_Device.maxSamples();
    msState.sampleShadingEnable = VK_TRUE;
    msState.minSampleShading = 0.2f;
    m_NormalDebugPipeline = std::make_unique<ShaderPipelineVk>(
            "Normal Debug Pipeline",
            NORMALDEBUG_SHADERS,
            std::unordered_set<BindingKey>{{0, 0}},
            *m_RenderPass,
            0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
            depthState,
            msState);


    depthState.testEnable = VK_FALSE;
    depthState.writeEnable = VK_FALSE;
    depthState.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_SkyboxPipeline = std::make_unique<ShaderPipelineVk>(
            "SkyboxShader",
            SKYBOX_SHADERS,
            std::unordered_set<BindingKey>{},
            *m_RenderPass,
            0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
            depthState,
            msState);


    m_GfxCmdPool = m_Device.createCommandPool(m_Device.GfxQueueIdx(),
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_TransferCmdPool = m_Device.createCommandPool(m_Device.TransferQueueIdx(),
                                                   VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    m_GfxCmdBuffers = vk::CommandBuffers(m_Device, m_GfxCmdPool->data(),
                                         VK_COMMAND_BUFFER_LEVEL_PRIMARY, maxImgCount);
    m_TransferCmdBuffers = vk::CommandBuffers(m_Device, m_TransferCmdPool->data(),
                                              VK_COMMAND_BUFFER_LEVEL_PRIMARY, maxImgCount);

    CreateImageResources(swapchain);

    CreateSynchronizationPrimitives();

    // Store cube vertices in host visible memory
    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkDeviceSize dataSize = sizeof(glm::vec3) * Mesh::s_CubeVertexPositions.size();
    m_BasicCubeVBO = std::make_unique<vk::Buffer>(m_Device,
                                                  std::set<uint32_t>{m_Device.GfxQueueIdx()},
                                                  dataSize,
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    m_BasicCubeMemory = std::make_unique<vk::DeviceMemory>(m_Device, m_Device,
                                                           std::vector<const vk::Buffer *>{m_BasicCubeVBO.get()},
                                                           memoryFlags);
    m_BasicCubeVBO->BindMemory(m_BasicCubeMemory->data(), 0);
    m_BasicCubeMemory->MapMemory(0, dataSize);
    std::memcpy(m_BasicCubeMemory->m_Mapped, Mesh::s_CubeVertexPositions.data(), dataSize);
    m_BasicCubeMemory->UnmapMemory();

   m_Viewport.x = 0;
   m_Viewport.y = static_cast<float>(extent.height);
   m_Viewport.width = static_cast<float>(extent.width);
   m_Viewport.height = -static_cast<float>(extent.height);
   m_Viewport.minDepth = 0.0f;
   m_Viewport.maxDepth = 1.0f;
   m_Scissor.extent = extent;

   InitializeTextureResources();
}


void RendererVk::CreateImageResources(const vk::Swapchain &swapchain) {
    m_FBOs.clear();

    m_ColorImage = vk::Image(m_Device,
                             {m_Device.GfxQueueIdx()},
                             swapchain.Extent(), 1,
                             VK_SAMPLE_COUNT_1_BIT,
                             VK_FORMAT_R32G32B32A32_SFLOAT,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                             VK_IMAGE_USAGE_SAMPLED_BIT);

    m_MSColorImage = vk::Image(m_Device, {m_Device.GfxQueueIdx()},
                               swapchain.Extent(), 1,
                               m_Device.maxSamples(),
                               VK_FORMAT_R32G32B32A32_SFLOAT,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    m_DepthImage = vk::Image(m_Device,
                             {m_Device.queueIndex(QueueFamily::GRAPHICS)},
                             swapchain.Extent(), 1,
                             m_Device.maxSamples(),
                             m_Device.findDepthFormat(), VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    m_MSColorImage.BindMemory(m_ImageMemory.data(), 0);

    auto offset = math::roundUp(m_MSColorImage.MemoryInfo().size, m_DepthImage.MemoryInfo().alignment);
    m_DepthImage.BindMemory(m_ImageMemory.data(), offset);

    offset = math::roundUp(offset + m_DepthImage.MemoryInfo().size, m_ColorImage.MemoryInfo().alignment);
    m_ColorImage.BindMemory(m_ImageMemory.data(), offset);

    m_MSColorImageView = vk::ImageView(m_Device, m_MSColorImage, VK_IMAGE_ASPECT_COLOR_BIT);
    m_ColorImageView = vk::ImageView(m_Device, m_ColorImage, VK_IMAGE_ASPECT_COLOR_BIT);
    m_DepthImageView = vk::ImageView(m_Device, m_DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

    std::vector<VkImageView> offscreenAttachments{m_ColorImageView.data(), m_DepthImageView.data()};
    if (m_Device.maxSamples() > VK_SAMPLE_COUNT_1_BIT) {
        offscreenAttachments.push_back(m_MSColorImageView.data());
    }
    m_OffscreenFBO = vk::Framebuffer(m_Device, m_RenderPass->data(), swapchain.Extent(), offscreenAttachments);

    for (const vk::ImageView &swapchainView : swapchain.ImageViews()) {
        m_FBOs.emplace_back(m_Device, m_PostprocessRenderPass->data(), swapchain.Extent(),
                            std::vector<VkImageView>{swapchainView.data()});
    }

    m_PostprocessPipeline->BindTextures2D({m_ColorImageView.data()}, BindingKey(0, 0));
}


void RendererVk::CreateSynchronizationPrimitives() {
    m_AcquireSemaphores.clear();
    m_ReleaseSemaphores.clear();
    m_Fences.clear();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_AcquireSemaphores.emplace_back(m_Device);
        m_ReleaseSemaphores.emplace_back(m_Device);
        m_Fences.emplace_back(m_Device,true);
    }
}


void RendererVk::RecreateSwapchain() {
    vkDeviceWaitIdle(m_Device);

    m_Context.RecreateSwapchain();
    const auto& swapchain = m_Context.Swapchain();
    assert(swapchain.ImageCount() == m_SwapchainImageCount);
//    uint32_t maxImgCount = swapchain.Capabilities().maxImageCount;

    CreateImageResources(swapchain);

//    CreateSynchronizationPrimitives();

//    m_FrameIndex = 0;

//    m_GfxCmdBuffers = vk::CommandBuffers(m_Device, m_GfxCmdPool->data(),
//                                         VK_COMMAND_BUFFER_LEVEL_PRIMARY, maxImgCount);
//    m_TransferCmdBuffers = vk::CommandBuffers(m_Device, m_TransferCmdPool->data(),
//                                              VK_COMMAND_BUFFER_LEVEL_PRIMARY, maxImgCount);

    VkExtent2D newExtent = swapchain.Extent();
    m_Viewport.x = 0;
    m_Viewport.y = static_cast<float>(newExtent.height);
    m_Viewport.width = static_cast<float>(newExtent.width);
    m_Viewport.height = -static_cast<float>(newExtent.height);
    m_Viewport.minDepth = 0.0f;
    m_Viewport.maxDepth = 1.0f;
    m_Scissor.extent = newExtent;
}


void RendererVk::AcquireNextImage() {
    auto &ctx = static_cast<GfxContextVk &>(m_Context);
    VkResult result = vkAcquireNextImageKHR(m_Device, ctx.Swapchain().data(), UINT64_MAX,
                                            m_AcquireSemaphores[m_FrameIndex].data(), nullptr,
                                            &m_ImageIndex);

    vkWaitForFences(m_Device, 1, m_Fences[m_FrameIndex].ptr(), VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, m_Fences[m_FrameIndex].ptr());

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//        std::cout << "OUT_OF_DATE" << std::endl;
//        RecreateSwapchain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swapchain image!");
    }
}

void RendererVk::DrawFrame() {
    static std::unordered_map<BindingKey, uint32_t> uniformObjectOffsets;

    vk::CommandBuffer primaryCmdBuffer(m_GfxCmdBuffers[m_ImageIndex]);
    primaryCmdBuffer.Begin();


    m_RenderPass->Begin(primaryCmdBuffer, m_OffscreenFBO);
    vkCmdSetViewport(primaryCmdBuffer.data(), 0, 1, &m_Viewport);
    vkCmdSetScissor(primaryCmdBuffer.data(), 0, 1, &m_Scissor);

    if (s_SkyboxEnabled && s_Skybox) {
        glm::mat4 PV = s_SceneCamera->GetProjection() * glm::mat4(glm::mat3(s_SceneCamera->GetView()));

        m_SkyboxPipeline->Bind(primaryCmdBuffer.data());
        m_SkyboxPipeline->BindDescriptorSets(m_ImageIndex, {});
        m_SkyboxPipeline->PushConstants(primaryCmdBuffer.data(), {VK_SHADER_STAGE_VERTEX_BIT, 0}, PV);
        m_SkyboxPipeline->PushConstants(primaryCmdBuffer.data(), {VK_SHADER_STAGE_FRAGMENT_BIT, 1}, s_SkyboxTexIdx);
        m_SkyboxPipeline->PushConstants(primaryCmdBuffer.data(), {VK_SHADER_STAGE_FRAGMENT_BIT, 2}, s_SkyboxLOD);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(primaryCmdBuffer.data(), 0, 1, m_BasicCubeVBO->ptr(), &offset);
        vkCmdDraw(primaryCmdBuffer.data(), Mesh::s_CubeVertexPositions.size(), 1, 0, 0);
    }

    uniformObjectOffsets.clear();
    RenderCommand *cmd = nullptr;
    ShaderPipelineVk *boundPipeline = nullptr;
    std::optional<uint32_t> materialID{};
    std::optional<uint32_t> materialInstanceID{};
    while ((cmd = m_CmdQueue.GetNextCommand()) != nullptr) {
        switch (cmd->m_Type) {
            case RenderCommand::Type::SET_CLEAR_COLOR: {
                auto clearColor = cmd->UnpackData<math::vec4>();
//                std::memcpy(m_ClearValues[0].color.float32, &clearColor, sizeof(math::vec4));
                break;
            }
            case RenderCommand::Type::SET_DEPTH_STENCIL: {
                auto depthStencil = cmd->UnpackData<DepthStencil>();
//                std::memcpy(&m_ClearValues[1].depthStencil, &depthStencil, sizeof(DepthStencil));
                break;
            }
            case RenderCommand::Type::SET_VIEWPORT: {
                auto viewport = cmd->UnpackData<Viewport>();
                std::memcpy(&m_Viewport, &viewport, sizeof(Viewport));
                break;
            }
            case RenderCommand::Type::SET_SCISSOR: {
                auto scissor = cmd->UnpackData<Scissor>();
                std::memcpy(&m_Scissor, &scissor, sizeof(Scissor));
                break;
            }
//            case RenderCommand::Type::BIND_VERTEX_BUFFER: {
//                auto payload = cmd->UnpackData<BindVertexBufferPayload>();
//                payload.vertexBuffer->Bind(primaryCmdBuffer.data(), payload.offset);
//                break;
//            }
//            case RenderCommand::Type::BIND_INDEX_BUFFER: {
//                auto payload = cmd->UnpackData<BindIndexBufferPayload>();
//                payload.indexBuffer->Bind(primaryCmdBuffer.data(), payload.offset);
//                break;
//            }
            case RenderCommand::Type::BIND_MATERIAL: {
                auto *material = cmd->UnpackData<Material *>();
                materialID = material->GetMaterialID();
                boundPipeline = static_cast<ShaderPipelineVk *>(&material->GetPipeline());
                boundPipeline->Bind(primaryCmdBuffer.data());
                boundPipeline->BindDescriptorSets(m_ImageIndex, materialID);
                break;
            }
            case RenderCommand::Type::BIND_MESH: {
                const auto *meshInstance = cmd->UnpackData<const MeshRenderer *>();
                const auto *mesh = meshInstance->GetMesh();
                auto it = m_MeshAllocations.find(mesh->MeshID());
                if (it == m_MeshAllocations.end())
                    assert(false);
                const auto &meshInfo = it->second;
                vkCmdBindVertexBuffers(primaryCmdBuffer.data(),
                                       0,
                                       1,
                                       meshInfo.buffer->bufferPtr(),
                                       &meshInfo.startOffset);

                if (!mesh->Indices().empty()) {
                    vkCmdBindIndexBuffer(primaryCmdBuffer.data(),
                                         meshInfo.buffer->buffer(),
                                         meshInfo.startOffset + mesh->VertexData().size(),
                                         VK_INDEX_TYPE_UINT32);
                }

                materialInstanceID = meshInstance->GetMaterialInstance().InstanceID();
//                auto materialID = meshInstance->GetMaterial();
//                boundPipeline->SetDynamicOffsets(instanceID);
//                boundPipeline->SetDynamicOffsets(mesh->GetMaterialObjectIdx());
                break;
            }
            case RenderCommand::Type::SET_UNIFORM_OFFSET: {
                auto payload = cmd->UnpackData<SetDynamicOffsetPayload>();
                uniformObjectOffsets[BindingKey(payload.set, payload.binding)] = payload.objectIndex;
                break;
            }
            case RenderCommand::Type::DRAW: {
                boundPipeline->SetDynamicOffsets(materialInstanceID.value(), uniformObjectOffsets);
                auto payload = cmd->UnpackData<DrawPayload>();
                vkCmdDraw(primaryCmdBuffer.data(),
                          payload.vertexCount,
                          payload.instanceCount,
                          payload.firstVertex,
                          payload.firstInstance);
                break;
            }
            case RenderCommand::Type::DRAW_INDEXED: {
                boundPipeline->SetDynamicOffsets(materialInstanceID.value(), uniformObjectOffsets);
                auto payload = cmd->UnpackData<DrawIndexedPayload>();
                vkCmdDrawIndexed(primaryCmdBuffer.data(),
                                 payload.indexCount,
                                 payload.instanceCount,
                                 payload.firstIndex,
                                 payload.vertexOffset,
                                 payload.firstInstance);

//                m_NormalDebugPipeline->Bind(primaryCmdBuffer.data());
//                boundPipeline->SetDynamicOffsets(materialInstanceID.value(), uniformObjectOffsets);
//                vkCmdDrawIndexed(primaryCmdBuffer.data(),
//                                 payload.indexCount,
//                                 payload.instanceCount,
//                                 payload.firstIndex,
//                                 payload.vertexOffset,
//                                 payload.firstInstance);
//
//                boundPipeline->Bind(primaryCmdBuffer.data());
//                boundPipeline->BindDescriptorSets(m_ImageIndex, materialID);
                break;
            }

            case RenderCommand::Type::NONE:
            case RenderCommand::Type::CLEAR:
                break;
        }
    }
    m_RenderPass->End(primaryCmdBuffer);


    /// Post processing part (full screen quad render + GUI)
    m_PostprocessRenderPass->Begin(primaryCmdBuffer, m_FBOs[m_ImageIndex]);

    VkViewport vp{0, 0, m_Viewport.width, m_Viewport.y, 0.0f, 1.0f};
    vkCmdSetViewport(primaryCmdBuffer.data(), 0, 1, &vp);
    vkCmdSetScissor(primaryCmdBuffer.data(), 0, 1, &m_Scissor);

    m_PostprocessPipeline->Bind(primaryCmdBuffer.data());
    m_PostprocessPipeline->BindDescriptorSets(m_ImageIndex, std::optional<uint32_t>());
    m_PostprocessPipeline->PushConstants(primaryCmdBuffer.data(), {VK_SHADER_STAGE_FRAGMENT_BIT, 0}, s_Exposure);
    vkCmdDraw(primaryCmdBuffer.data(), 3, 1, 0, 0);

    if (m_ImGuiLayer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), primaryCmdBuffer.data());
    }
//    /// ImGui fucks up viewport and scissor and doesn't restore it after the draw -_-
//    /// TODO: write my own ImGuI renderer
//    vkCmdSetViewport(primaryCmdBuffer.data(), 0, 1, &m_Viewport);
//    vkCmdSetScissor(primaryCmdBuffer.data(), 0, 1, &m_Scissor);

    m_PostprocessRenderPass->End(primaryCmdBuffer);
    primaryCmdBuffer.End();


    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = m_AcquireSemaphores[m_FrameIndex].ptr();
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = primaryCmdBuffer.ptr();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = m_ReleaseSemaphores[m_FrameIndex].ptr();

    VkResult result = vkQueueSubmit(m_Device.GfxQueue(), 1, &submitInfo, m_Fences[m_FrameIndex].data());
    if (result != VK_SUCCESS) {
        std::ostringstream msg;
        msg << "[vkQueueSubmit] Failed to submit draw command buffer, error code: '" << result << "'";
        throw std::runtime_error(msg.str().c_str());
    }

    auto &ctx = static_cast<GfxContextVk &>(m_Context);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = m_ReleaseSemaphores[m_FrameIndex].ptr();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = ctx.Swapchain().ptr();
    presentInfo.pImageIndices = &m_ImageIndex;

    result = vkQueuePresentKHR(m_Device.GfxQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
//        std::cout << "OUT_OF_DATE || SUBOPTIMAL" << std::endl;
//        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


void RendererVk::impl_OnWindowResize(WindowResizeEvent &) {
    RecreateSwapchain();
}


void RendererVk::impl_WaitIdle() const {
    vkDeviceWaitIdle(m_Device);
    std::cout << currentTime() << "[Renderer] Idle state" << std::endl;
}


void RendererVk::impl_SetSkybox(const TextureCubemap *skybox) {
    auto texIndices = m_SkyboxPipeline->BindCubemaps({skybox}, BindingKey(0, 0));
    s_SkyboxTexIdx = texIndices.back();
}


void RendererVk::impl_FlushStagedData() {
    /// TODO: sort by data type and distribute into multiple device buffers, etc...

    std::array<VkBufferMemoryBarrier, 1> bufferBarriers = {};
    vk::Semaphore transferSemaphore(m_Device);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    {
        vk::CommandBuffer transferCmdBuffer(m_TransferCmdBuffers[m_ImageIndex]);
        transferCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        auto copyRegions = m_StageBuffer.CopyRegions();
        VkBufferCopy dstRegion = m_MeshDeviceBuffer.TransferData(transferCmdBuffer, m_StageBuffer, copyRegions);

        VkDeviceSize dstSubDataOffset = dstRegion.dstOffset;
        while (!m_StageBuffer.IsEmpty()) {
            vk::RingStageBuffer::DataInfo metadata = m_StageBuffer.PopMetadata();
            switch (metadata.dataType) {
                case vk::RingStageBuffer::DataType::MESH_DATA: {
                    auto it = m_MeshAllocations.find(metadata.resourceID);
                    if (it == m_MeshAllocations.end())
                        assert(false);

                    MeshAllocationMetadata &meshAllocInfo = it->second;
                    meshAllocInfo.buffer = &m_MeshDeviceBuffer;
                    meshAllocInfo.startOffset = dstSubDataOffset;
                    break;
                }

                case vk::RingStageBuffer::DataType::TEXTURE_DATA:
                    assert(false);

                default:
                    assert(false);
            }
            dstSubDataOffset += metadata.dataSize;
        }

        bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
        bufferBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
//        bufferBarriers[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        bufferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarriers[0].dstAccessMask = 0;
        bufferBarriers[0].buffer = m_MeshDeviceBuffer.buffer();
        bufferBarriers[0].size = dstRegion.size;
        bufferBarriers[0].offset = dstRegion.dstOffset;

        vkCmdPipelineBarrier(transferCmdBuffer.data(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                             0, nullptr,
                             bufferBarriers.size(), bufferBarriers.data(),
                             0, nullptr
        );

        transferCmdBuffer.End();

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = transferSemaphore.ptr();
        transferCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::TRANSFER));
    }

    /* Acquisition phase */
    {
        vk::CommandBuffer gfxCmdBuffer(m_GfxCmdBuffers[m_ImageIndex]);
        gfxCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        bufferBarriers[0].srcAccessMask = 0;
        bufferBarriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        vkCmdPipelineBarrier(gfxCmdBuffer.data(),
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
                             0, nullptr,
                             bufferBarriers.size(), bufferBarriers.data(),
                             0, nullptr
        );

        gfxCmdBuffer.End();

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = transferSemaphore.ptr();
        submitInfo.pWaitDstStageMask = &waitStage;
        gfxCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::GRAPHICS));

        vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));
    }
}

