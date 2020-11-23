#include "ImGuiLayerVk.h"

#include <Engine/Application.h>
#include <examples/imgui_impl_vulkan.h>
#include <examples/imgui_impl_glfw.h>

#include "RendererVk.h"
#include "GraphicsContextVk.h"


ImGuiLayerVk::~ImGuiLayerVk() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}


static void check_vk_result(VkResult err) {
    if (err == 0) return;
    printf("VkResult %d\n", err);
    if (err < 0)
        abort();
}


ImGuiLayerVk::ImGuiLayerVk() :
        ImGuiLayer(),
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_RenderPass(nullptr) {}


void ImGuiLayerVk::OnAttach(const LayerStack *stack) {
    ImGuiLayer::OnAttach(stack);
    ImGuiLayer::ConfigureImGui(m_Context.FramebufferSize());
    InitResources();
}


void ImGuiLayerVk::InitResources() {
    auto imgCount = m_Context.Swapchain().ImageCount();

    std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
    };

    m_DescriptorPool = m_Device.createDescriptorPool(poolSizes, 1000 * poolSizes.size(),
                                                     VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    m_CmdPool = m_Device.createCommandPool(m_Device.GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_CmdBuffers = m_Device.createCommandBuffers(*m_CmdPool, imgCount);

    m_RenderPass = (VkRenderPass) Renderer::GetRenderPass().VkHandle();

//    auto extent = m_Context.Swapchain().Extent();
//    const auto& imgViews = m_Context.Swapchain().ImageViews();
//    m_Framebuffers.resize(imgCount);
//    for (uint32_t i = 0; i < imgCount; i++) {
//        auto* buffer = m_Device.createFramebuffer(extent, m_RenderPass, {imgViews[i].data()});
//        m_Framebuffers[i] = buffer;
//    }

//    {
//        VkAttachmentDescription attachment = {};
//        attachment.format = m_Context.Swapchain().ImageFormat();
//        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
//        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
//        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//        VkAttachmentReference color_attachment = {};
//        color_attachment.attachment = 0;
//        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//        VkSubpassDescription subpass = {};
//        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//        subpass.colorAttachmentCount = 1;
//        subpass.pColorAttachments = &color_attachment;
//
//        VkSubpassDependency dependency = {};
//        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//        dependency.dstSubpass = 0;
//        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//
//        VkRenderPassCreateInfo info = {};
//        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//        info.attachmentCount = 1;
//        info.pAttachments = &attachment;
//        info.subpassCount = 1;
//        info.pSubpasses = &subpass;
//        info.dependencyCount = 1;
//        info.pDependencies = &dependency;
//        if (vkCreateRenderPass(ctx.GetDevice(), &info, nullptr, &m_RenderPass) != VK_SUCCESS) {
//            throw std::runtime_error("Could not create Dear ImGui's render pass");
//        }
//    }

    // Setup Platform/Renderer bindings
    auto *window = static_cast<GLFWwindow *>(Application::GetWindow().GetNativeHandle());
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Context.Instance().data();
    init_info.PhysicalDevice = m_Device;
    init_info.Device = m_Device;
    init_info.QueueFamily = m_Device.GfxQueueIdx();
    init_info.Queue = m_Device.GfxQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_DescriptorPool->data();
    init_info.Allocator = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = imgCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, m_RenderPass);

    //TODO: causes validation error due to secondary level, will fix
    // when renderer architecture allows for it
    vk::CommandBuffer cmdBuffer = m_CmdBuffers->get(0);
    cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer.data());
    cmdBuffer.End();
    cmdBuffer.Submit(m_Device.GfxQueue());
    vkQueueWaitIdle(m_Device.GfxQueue());


//
//    m_TextureImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
//    copyBufferToImage(setupCmdBuffer, stagingBuffer, m_TextureImage);
//    m_TextureImage.GenerateMipmaps(device, setupCmdBuffer);
//    //m_TextureImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//
//
//    setupCmdBuffer.End();
//    setupCmdBuffer.Submit(device.GfxQueue());
//    vkQueueWaitIdle(device.GfxQueue());


//    auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
//    ImGuiIO &io = ImGui::GetIO();
//
//    // Create font texture
//    unsigned char *fontData(nullptr);
//    int texSize[2] = {};
//    io.Fonts->GetTexDataAsRGBA32(&fontData, &texSize[0], &texSize[1]);
//    m_FontData = Texture2D::Create(fontData, texSize[0], texSize[1]);
//    m_FontData->Upload();
//
//    PushConstant pushBlock = {sizeof(PushConstBlock)};
//
//    VertexLayout vertexLayout{
//            VertexAttribute(ShaderType::Float2),
//            VertexAttribute(ShaderType::Float2),
//            VertexAttribute(ShaderType::UInt)
//    };
//
//    DescriptorLayout descriptorLayout{
//            DescriptorBinding(DescriptorType::Texture)
//    };
//
//    std::unique_ptr<ShaderProgram> fragShader(ShaderProgram::Create("shaders/ui.frag.spv"));
//    std::unique_ptr<ShaderProgram> vertexShader(ShaderProgram::Create("shaders/ui.vert.spv"));
//
//    m_Pipeline = Pipeline::Create(renderPass, *vertexShader, *fragShader, vertexLayout, descriptorLayout, {pushBlock},
//                                  false);
//    m_Pipeline->BindTextures(*m_FontData, 0);
//
//    auto imgCount = gfxContext.Swapchain().ImageCount();
//    m_VertexBuffers.resize(imgCount);
//    m_IndexBuffers.resize(imgCount);
//    m_VertexMemories.resize(imgCount);
//    m_IndexMemories.resize(imgCount);
}


void ImGuiLayerVk::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayerVk::EndFrame() {
    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}


// Starts a new imGui frame and sets up windows and ui elements
//void ImGuiLayerVk::OnDraw() {
//    ImGui_ImplVulkan_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//    m_DrawCallback();
//    ImGui::ShowDemoWindow();
//    ImGui::Render();
//
//    // Update and Render additional Platform Windows
//    ImGuiIO &io = ImGui::GetIO();
//    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
//        ImGui::UpdatePlatformWindows();
//        ImGui::RenderPlatformWindowsDefault();
//    }
//
//    auto extent = m_Context.Swapchain().Extent();
//    VkViewport viewport = {};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = static_cast<float>(extent.width);
//    viewport.height = static_cast<float>(extent.height);
//    viewport.minDepth = 0.0f;
//    viewport.maxDepth = 1.0f;
//
//    VkRect2D scissor = {};
//    scissor.offset = {0, 0};
//    scissor.extent = extent;
//
//    vk::CommandBuffer cmdBuffer = m_CmdBuffers->get(imageIndex);
//    VkCommandBufferBeginInfo beginInfo = {};
//    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
//    beginInfo.pInheritanceInfo = &info;
//
//    cmdBuffer.Begin(beginInfo);
//
//    vkCmdSetViewport(cmdBuffer.data(), 0, 1, &viewport);
//    vkCmdSetScissor(cmdBuffer.data(), 0, 1, &scissor);
//
//    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer.data());
//
//    vkCmdEndRenderPass(cmdBuffer.data());
//    cmdBuffer.End();
//    return cmdBuffer.data();
//
//    UpdateBuffers(m_Renderer.GetCurrentImageIndex());
//}


// Update vertex and index buffer containing the imGui elements when required
//void ImGuiLayer::UpdateBuffers(size_t frameIndex) {
//    ImDrawData *imDrawData = ImGui::GetDrawData();
//
//    // Note: Alignment is done inside buffer creation
//    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
//    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
//
//    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
//        return;
//    }
//
//
//    // Update buffers only if vertex or index count has been changed compared to current buffer size
//    auto &device = m_Renderer.GetDevice();
//    if (vertexBufferSize > m_VertexBuffers[frameIndex].Size()) {
//        m_VertexMemories[frameIndex].UnmapMemory();
//        m_VertexBuffers[frameIndex] = device.createBuffer({device.queueIndex(QueueFamily::GRAPHICS)},
//                                                          vertexBufferSize * 2,
//                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
//        m_VertexMemories[frameIndex] = device.allocateBufferMemory(m_VertexBuffers[frameIndex],
//                                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//        m_VertexBuffers[frameIndex].BindMemory(m_VertexMemories[frameIndex].data(), 0);
//        m_VertexMemories[frameIndex].MapMemory(0, vertexBufferSize * 2);
//    }
//
//    if (indexBufferSize > m_IndexBuffers[frameIndex].Size()) {
//        m_IndexMemories[frameIndex].UnmapMemory();
//        m_IndexBuffers[frameIndex] = device.createBuffer({device.queueIndex(QueueFamily::GRAPHICS)},
//                                                         indexBufferSize * 2,
//                                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
//        m_IndexMemories[frameIndex] = device.allocateBufferMemory(m_IndexBuffers[frameIndex],
//                                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//        m_IndexBuffers[frameIndex].BindMemory(m_IndexMemories[frameIndex].data(), 0);
//        m_IndexMemories[frameIndex].MapMemory(0, indexBufferSize);
//    }
//
//    // Upload data
//    auto *vtxDst = (ImDrawVert *) m_VertexMemories[frameIndex].m_Mapping;
//    auto *idxDst = (ImDrawIdx *) m_IndexMemories[frameIndex].m_Mapping;
//    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
//        const ImDrawList *cmd_list = imDrawData->CmdLists[n];
//        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//        vtxDst += cmd_list->VtxBuffer.Size;
//        idxDst += cmd_list->IdxBuffer.Size;
//    }
//
//    // Flush to make writes visible to GPU
//    m_VertexMemories[frameIndex].Flush(0, VK_WHOLE_SIZE);
//    m_IndexMemories[frameIndex].Flush(0, VK_WHOLE_SIZE);
//}

// Draw current imGui frame into a command buffer
auto ImGuiLayerVk::RecordBuffer(size_t index) -> VkCommandBuffer {
    auto extent = m_Context.Swapchain().Extent();
    vk::CommandBuffer cmdBuffer = m_CmdBuffers->get(index);
    cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        std::array<VkClearValue, 1> clearValues = {
                {{VkClearColorValue{{0.0f, 0.0f, 0.0f, 1.0f}}}}
        };

        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = m_RenderPass;
        info.framebuffer = m_Framebuffers[index]->data();
        info.renderArea.extent.width = extent.width;
        info.renderArea.extent.height = extent.height;
        info.clearValueCount = clearValues.size();
        info.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(cmdBuffer.data(), &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer.data());

    // Submit command buffer
    vkCmdEndRenderPass(cmdBuffer.data());
    cmdBuffer.End();
    return cmdBuffer.data();

//    ImGuiIO &io = ImGui::GetIO();
//
//    m_Pipeline->Bind(commandBuffer, index);
//
//    VkViewport viewport = {};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = static_cast<float>(ImGui::GetIO().DisplaySize.x);
//    viewport.height = static_cast<float>(ImGui::GetIO().DisplaySize.y);
//    viewport.minDepth = 0.0f;
//    viewport.maxDepth = 1.0f;
//    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//
//    // UI scale and translate via push constants
//    pushConstBlock.scale = math::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
//    pushConstBlock.translate = math::vec2(-1.0f);
//    m_Pipeline->PushConstants(commandBuffer, 0, sizeof(PushConstBlock), &pushConstBlock);
//
//    // Render commands
//    ImDrawData *imDrawData = ImGui::GetDrawData();
//    int32_t vertexOffset = 0;
//    int32_t indexOffset = 0;
//
//    if (imDrawData->CmdListsCount > 0) {
//
//        VkDeviceSize offsets[1] = {0};
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, m_VertexBuffers[index].ptr(), offsets);
//        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffers[index].data(), 0, VK_INDEX_TYPE_UINT16);
//
//        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
//            const ImDrawList *cmd_list = imDrawData->CmdLists[i];
//            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
//                const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
//                VkRect2D scissorRect;
//                scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
//                scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
//                scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
//                scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
//                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
//                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
//                indexOffset += pcmd->ElemCount;
//            }
//            vertexOffset += cmd_list->VtxBuffer.Size;
//        }
//    }
}
