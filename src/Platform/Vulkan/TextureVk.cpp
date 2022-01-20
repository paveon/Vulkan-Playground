#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "TextureVk.h"
#include "Engine/Application.h"
#include "GraphicsContextVk.h"
#include "RenderPassVk.h"
#include "ShaderPipelineVk.h"

using namespace vk;

std::unique_ptr<RenderPassVk> TextureCubemapVk::s_CubemapRenderpass;
std::unique_ptr<ShaderPipelineVk> TextureCubemapVk::s_CubemapPipeline;
std::unique_ptr<ShaderPipelineVk> TextureCubemapVk::s_IrradiancePipeline;
std::unique_ptr<ShaderPipelineVk> TextureCubemapVk::s_PrefilterPipeline;
std::unique_ptr<vk::Buffer> TextureCubemapVk::s_CubeVertexBuffer;
std::unique_ptr<vk::DeviceMemory> TextureCubemapVk::s_CubeVertexMemory;
std::array<glm::mat4, 6> TextureCubemapVk::s_CaptureViews;

std::unique_ptr<RenderPassVk> Texture2DVk::s_Renderpass;
std::unique_ptr<ShaderPipelineVk> Texture2DVk::s_BrdfLutPipeline;


auto PrepareTextureImage(uint32_t width,
                         uint32_t height,
                         uint32_t maxMipLevels,
                         VkFormat format,
                         VkImageCreateFlags flags,
                         VkImageUsageFlags usage) -> vk::Image * {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   uint32_t mipLevels = std::min(TextureCubemap::MipLevels(width, height), maxMipLevels);

   VkImageCreateInfo imageInfo = {};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.format = format;
   imageInfo.extent.width = width;
   imageInfo.extent.height = height;
   imageInfo.extent.depth = 1;
   imageInfo.mipLevels = mipLevels;
   imageInfo.arrayLayers = (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1;
   imageInfo.flags = flags;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.usage = usage;
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   return device.createImage(imageInfo);
}


void Texture2DVk::InitResources() {
   s_Renderpass = std::make_unique<RenderPassVk>(
           std::vector<VkAttachmentDescription>{
                   VkAttachmentDescription{
                           0,
                           VK_FORMAT_R16G16_SFLOAT,
                           VK_SAMPLE_COUNT_1_BIT,
                           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                           VK_ATTACHMENT_STORE_OP_STORE,
                           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                           VK_ATTACHMENT_STORE_OP_DONT_CARE,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   }
           },
           std::vector<VkSubpassDescription>{},
           std::vector<VkSubpassDependency>{},
           std::vector<uint32_t>{0},
           std::optional<uint32_t>{}
   );

   const std::map<ShaderType, const char *> BRDF_LUT_GENERATION_SHADERS{
           {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/fullscreenQuad.vert.spv"},
           {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/BRDF_LUT_Generation.frag.spv"}
   };

   MultisampleState msState{};
   msState.sampleCount = VK_SAMPLE_COUNT_1_BIT;

   s_BrdfLutPipeline = std::make_unique<ShaderPipelineVk>(
           "BRDF LUT Generation Pipeline",
           BRDF_LUT_GENERATION_SHADERS,
           std::unordered_set<BindingKey>{},
           *s_Renderpass,
           0,
           VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
           std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
           DepthState{},
           msState);
}


void Texture2DVk::ReleaseResources() {
   s_BrdfLutPipeline.reset();
   s_Renderpass.reset();
}


Texture2DVk::Texture2DVk(const u_char *data, uint32_t width, uint32_t height, uint32_t channels, VkFormat format) :
        Texture2D(data, width, height, channels, format),
        m_TextureImage(PrepareTextureImage(width, height, MipLevels(), format, 0,
                                           VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {}


Texture2DVk::Texture2DVk(uint32_t resolution, uint32_t channels, VkFormat format)
        : Texture2D(resolution, resolution, channels, format),
          m_TextureImage(PrepareTextureImage(resolution, resolution, 1, format, 0,
                                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {}


void Texture2DVk::Upload() {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());

   Device &device = gfxContext.GetDevice();

   m_TextureMemory = device.allocateImageMemory({m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   m_TextureImage->BindMemory(m_TextureMemory->data(), 0);
//    vkBindImageMemory(device, m_TextureImage->data(), m_TextureMemory->data(), 0);

   m_TextureView = device.createImageView(*m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);
//   m_TextureView = m_TextureImage->createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

   // Create target image for copy
   StagingBuffer stagingBuffer(&device, m_Data.data(), m_Data.size());

   CommandPool pool(device, device.GfxQueueIdx());
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
   setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, VK_ACCESS_TRANSFER_WRITE_BIT, {});

   copyBufferToImage(setupCmdBuffer, stagingBuffer, *m_TextureImage);

   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT,
                                {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}});

   m_TextureImage->GenerateMipmaps(device, setupCmdBuffer);

   setupCmdBuffer.End();
   setupCmdBuffer.Submit(device.GfxQueue());
   vkQueueWaitIdle(device.GfxQueue());
}


std::unique_ptr<Texture2DVk> Texture2DVk::GenerateBrdfLut(uint32_t resolution) {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   auto lut = std::make_unique<Texture2DVk>(resolution, 2, VK_FORMAT_R16G16_SFLOAT);

   VkExtent2D extent{resolution, resolution};

   // Destination Irradiance resources
   lut->m_TextureMemory = device.allocateImageMemory({lut->m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   lut->m_TextureImage->BindMemory(lut->m_TextureMemory->data(), 0);
   lut->m_TextureView = device.createImageView(*lut->m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);

   VkViewport viewport{
           0.0f,
           0.0f,
           static_cast<float>(resolution),
           static_cast<float>(resolution),
           0.0f,
           1.0f
   };
   VkRect2D scissor{};
   scissor.extent = extent;

   vk::Framebuffer framebuffer(device, s_Renderpass->data(), extent, {lut->m_TextureView->data()});

   CommandPool pool(device, device.GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
   setupCmdBuffer.Begin();

   s_Renderpass->Begin(setupCmdBuffer, framebuffer);
   s_BrdfLutPipeline->Bind(setupCmdBuffer.data());

   vkCmdSetViewport(setupCmdBuffer.data(), 0, 1, &viewport);
   vkCmdSetScissor(setupCmdBuffer.data(), 0, 1, &scissor);

   vkCmdDraw(setupCmdBuffer.data(), 3, 1, 0, 0);

   s_Renderpass->End(setupCmdBuffer);

   setupCmdBuffer.End();
   setupCmdBuffer.Submit(device.GfxQueue());
   vkQueueWaitIdle(device.GfxQueue());

   return lut;
}


void TextureCubemapVk::InitResources() {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   VkClearValue clearDepthValue;
   clearDepthValue.depthStencil = {1.0f, 0};

//        VkClearValue clearColorValue;
//        clearColorValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};
//        for (uint32_t i = 0; i < faceCount; i++) {
//            cubemapRenderpass->SetClearValue(i, clearColorValue);
//        }

   s_CubemapRenderpass = std::make_unique<RenderPassVk>(
           std::vector<VkAttachmentDescription>{
                   VkAttachmentDescription{
                           0,
                           VK_FORMAT_R32G32B32A32_SFLOAT,
                           VK_SAMPLE_COUNT_1_BIT,
                           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                           VK_ATTACHMENT_STORE_OP_STORE,
                           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                           VK_ATTACHMENT_STORE_OP_DONT_CARE,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   },
                   VkAttachmentDescription{
                           0,
                           device.findDepthFormat(),
                           VK_SAMPLE_COUNT_1_BIT,
                           VK_ATTACHMENT_LOAD_OP_CLEAR,
                           VK_ATTACHMENT_STORE_OP_DONT_CARE,
                           VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                           VK_ATTACHMENT_STORE_OP_DONT_CARE,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                   }
           },
           std::vector<VkSubpassDescription>{},
           std::vector<VkSubpassDependency>{},
           std::vector<uint32_t>{0},
           std::optional<uint32_t>{1}
   );

   s_CubemapRenderpass->SetClearValue(1, clearDepthValue);

   glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
   s_CaptureViews = std::array{
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
           proj * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
   };

   const std::map<ShaderType, const char *> CUBEMAP_GENERATION_SHADERS{
           {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/cubemapGeneration.vert.spv"},
           {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/cubemapGeneration.frag.spv"}
   };

   const std::map<ShaderType, const char *> IRRADIANCE_GENERATION_SHADERS{
           {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/cubemapGeneration.vert.spv"},
           {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/irradianceGeneration.frag.spv"}
   };

   const std::map<ShaderType, const char *> PREFILTERING_SHADERS{
           {ShaderType::VERTEX_SHADER,   BASE_DIR "/shaders/cubemapGeneration.vert.spv"},
           {ShaderType::FRAGMENT_SHADER, BASE_DIR "/shaders/environmentMapPrefilter.frag.spv"}
   };

   MultisampleState msState{};
   msState.sampleCount = VK_SAMPLE_COUNT_1_BIT;

   DepthState depthState{};
   depthState.testEnable = VK_TRUE;
   depthState.writeEnable = VK_TRUE;
   depthState.compareOp = VK_COMPARE_OP_LESS;
   depthState.min = 0.0f;
   depthState.max = 1.0f;
   s_CubemapPipeline = std::make_unique<ShaderPipelineVk>(
           std::string("Cubemap Generation Pipeline"),
           CUBEMAP_GENERATION_SHADERS,
           std::unordered_set<BindingKey>{},
           *s_CubemapRenderpass,
           0,
           VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
           std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
           depthState,
           msState);

   s_IrradiancePipeline = std::make_unique<ShaderPipelineVk>(
           std::string("Irradiance Generation Pipeline"),
           IRRADIANCE_GENERATION_SHADERS,
           std::unordered_set<BindingKey>{},
           *s_CubemapRenderpass,
           0,
           VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
           std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
           depthState,
           msState);

   s_PrefilterPipeline = std::make_unique<ShaderPipelineVk>(
           std::string("Environment Map Prefiltering Pipeline"),
           PREFILTERING_SHADERS,
           std::unordered_set<BindingKey>{},
           *s_CubemapRenderpass,
           0,
           VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
           std::make_pair(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE),
           depthState,
           msState);


   // Store cube vertices in host visible memory
   VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

   VkDeviceSize dataSize = sizeof(glm::vec3) * Mesh::s_CubeVertexPositions.size();
   s_CubeVertexBuffer = std::make_unique<vk::Buffer>(device,
                                                     std::set<uint32_t>{device.GfxQueueIdx()},
                                                     dataSize,
                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

   s_CubeVertexMemory = std::make_unique<vk::DeviceMemory>(device, device,
                                                           std::vector<const Buffer *>{s_CubeVertexBuffer.get()},
                                                           memoryFlags);
   s_CubeVertexBuffer->BindMemory(s_CubeVertexMemory->data(), 0);
   s_CubeVertexMemory->MapMemory(0, dataSize);
   std::memcpy(s_CubeVertexMemory->m_Mapped, Mesh::s_CubeVertexPositions.data(), dataSize);
   s_CubeVertexMemory->UnmapMemory();
}


void TextureCubemapVk::ReleaseResources() {
   s_CubeVertexBuffer.reset();
   s_CubeVertexMemory.reset();
   s_PrefilterPipeline.reset();
   s_IrradiancePipeline.reset();
   s_CubemapPipeline.reset();
   s_CubemapRenderpass.reset();
}


TextureCubemapVk::TextureCubemapVk(std::array<u_char *, 6> data, uint32_t width, uint32_t height, uint32_t channels)
        : TextureCubemap(data, width, height, channels),
          m_TextureImage(PrepareTextureImage(m_Width, m_Height, MipLevels(m_Width, m_Height),
                                             VK_FORMAT_R8G8B8A8_SRGB,
                                             VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                             VK_IMAGE_USAGE_SAMPLED_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {}


TextureCubemapVk::TextureCubemapVk(const float *data, uint32_t width, uint32_t height, uint32_t channels,
                                   uint32_t faceResolution)
        : TextureCubemap(data, width, height, channels),
          m_TextureImage(PrepareTextureImage(faceResolution, faceResolution,
                                             MipLevels(faceResolution, faceResolution),
                                             VK_FORMAT_R32G32B32A32_SFLOAT,
                                             VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                             VK_IMAGE_USAGE_SAMPLED_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {}


TextureCubemapVk::TextureCubemapVk(const u_char *data, uint32_t width, uint32_t height, uint32_t channels)
        : TextureCubemap(data, width, height, channels),
          m_TextureImage(PrepareTextureImage(512, 512, 1,
                                             VK_FORMAT_R8G8B8A8_SRGB,
                                             VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                             VK_IMAGE_USAGE_SAMPLED_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {}


TextureCubemapVk::TextureCubemapVk(uint32_t width, uint32_t height, uint32_t maxMipLevels) :
        TextureCubemap(width, height),
        m_TextureImage(PrepareTextureImage(width, width, maxMipLevels,
                                           VK_FORMAT_R32G32B32A32_SFLOAT,
                                           VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                           VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {}


void TextureCubemapVk::Upload() {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();
   m_TextureMemory = device.allocateImageMemory({m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   m_TextureImage->BindMemory(m_TextureMemory->data(), 0);
//    vkBindImageMemory(device, m_TextureImage->data(), m_TextureMemory->data(), 0);

   m_TextureView = device.createImageView(*m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);
//   m_TextureView = m_TextureImage->createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

   // Create target image for copy
   StagingBuffer stagingBuffer(&device, m_Data.data(), m_Data.size());

   CommandPool pool(device, device.GfxQueueIdx());
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
   setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, VK_ACCESS_TRANSFER_WRITE_BIT, {});

   copyBufferToImage(setupCmdBuffer, stagingBuffer, *m_TextureImage);

   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT,
                                {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6}});
   m_TextureImage->GenerateMipmaps(device, setupCmdBuffer);


   setupCmdBuffer.End();
   setupCmdBuffer.Submit(device.GfxQueue());
   vkQueueWaitIdle(device.GfxQueue());
}


void TextureCubemapVk::HDRtoCubemap() {
   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   if (!s_CubemapRenderpass) {
      TextureCubemapVk::InitResources();
   }

   const size_t cubeFaceCount = 6;
   const uint32_t cubeVertexCount = 36;

   VkExtent2D faceExtent{m_TextureImage->Info().extent.height, m_TextureImage->Info().extent.height};

   VkImageCreateInfo sourceImageInfo = {};
   sourceImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   sourceImageInfo.imageType = VK_IMAGE_TYPE_2D;
   sourceImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
   sourceImageInfo.extent.width = m_Width;
   sourceImageInfo.extent.height = m_Height;
   sourceImageInfo.extent.depth = 1;
   sourceImageInfo.mipLevels = MipLevels(m_Width, m_Height);
   sourceImageInfo.arrayLayers = 1;
   sourceImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   sourceImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   sourceImageInfo.usage =
           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   sourceImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   sourceImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

   vk::Image hdrSource(device, sourceImageInfo);
   vk::DeviceMemory hdrSourceMemory(device, device, {&hdrSource}, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   hdrSource.BindMemory(hdrSourceMemory.data(), 0);
   vk::ImageView hdrSourceView(device, hdrSource, VK_IMAGE_ASPECT_COLOR_BIT);

   StagingBuffer stagingBuffer(&device, m_Data.data(), m_Data.size());
   CommandPool pool(device, device.GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
   setupCmdBuffer.Begin();

   hdrSource.ChangeLayout(setupCmdBuffer,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0, VK_ACCESS_TRANSFER_WRITE_BIT, {});

   copyBufferToImage(setupCmdBuffer, stagingBuffer, hdrSource);

   hdrSource.ChangeLayout(setupCmdBuffer,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_TRANSFER_READ_BIT,
                          {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}});

   hdrSource.GenerateMipmaps(device, setupCmdBuffer);

   // Depth image
   vk::Image depthImage(device, {device.GfxQueueIdx()},
                        faceExtent, 1,
                        VK_SAMPLE_COUNT_1_BIT,
                        device.findDepthFormat(),
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

   vk::DeviceMemory depthImageMemory(device, device, {&depthImage}, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   depthImage.BindMemory(depthImageMemory.data(), 0);
   vk::ImageView depthImageView(device, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);


   // Destination Cubemap Image
   m_TextureMemory = device.allocateImageMemory({m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   m_TextureImage->BindMemory(m_TextureMemory->data(), 0);
   m_TextureView = device.createImageView(*m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);

   VkImageViewCreateInfo faceSubViewCreateInfo = {};
   faceSubViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   faceSubViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   faceSubViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
   faceSubViewCreateInfo.image = m_TextureImage->data();
   faceSubViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   faceSubViewCreateInfo.subresourceRange.baseMipLevel = 0;
   faceSubViewCreateInfo.subresourceRange.levelCount = 1;
   faceSubViewCreateInfo.subresourceRange.layerCount = 1;

   std::vector<ImageView> faceSubviews;
   std::vector<vk::Framebuffer> faceFramebuffers;
   for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
      faceSubViewCreateInfo.subresourceRange.baseArrayLayer = faceIdx;
      faceSubviews.emplace_back(device, faceSubViewCreateInfo);
      faceFramebuffers.emplace_back(device, s_CubemapRenderpass->data(), faceExtent,
                                    std::vector{faceSubviews.back().data(), depthImageView.data()});
   }

   s_CubemapPipeline->BindTextures2D({hdrSourceView.data()}, BindingKey(0, 0));

   VkViewport viewport{
           0.0f,
           0.0f,
           static_cast<float>(faceExtent.width),
           static_cast<float>(faceExtent.height),
           0.0f,
           1.0f
   };
   VkRect2D scissor{};
   scissor.extent = faceExtent;


   // Render part of the HDR texture to each face of the cubemap
   for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
      s_CubemapRenderpass->Begin(setupCmdBuffer, faceFramebuffers[faceIdx]);
      s_CubemapPipeline->Bind(setupCmdBuffer.data());
      s_CubemapPipeline->BindDescriptorSets(0, {});

      vkCmdSetViewport(setupCmdBuffer.data(), 0, 1, &viewport);
      vkCmdSetScissor(setupCmdBuffer.data(), 0, 1, &scissor);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(setupCmdBuffer.data(), 0, 1, s_CubeVertexBuffer->ptr(), &offset);

      s_CubemapPipeline->PushConstants(setupCmdBuffer.data(),
                                       {VK_SHADER_STAGE_VERTEX_BIT, 0}, s_CaptureViews[faceIdx]);
      vkCmdDraw(setupCmdBuffer.data(), cubeVertexCount, 1, 0, 0);

      s_CubemapRenderpass->End(setupCmdBuffer);
   }

   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT,
                                {{
                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                         0,
                                         1,
                                         0,
                                         cubeFaceCount}
                                }
   );

   /// Higher MIP levels were not used before (undefined layout) and
   /// the layout transition doesn't have to wait for rendering to finish
   m_TextureImage->ChangeLayout(setupCmdBuffer,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                {{
                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                         1,
                                         m_TextureImage->MipLevels() - 1,
                                         0,
                                         cubeFaceCount}
                                }
   );

   m_TextureImage->GenerateMipmaps(device, setupCmdBuffer);
   setupCmdBuffer.End();
   setupCmdBuffer.Submit(device.GfxQueue());
   vkQueueWaitIdle(device.GfxQueue());
}


auto TextureCubemapVk::CreateIrradianceCubemap(uint32_t resolution) -> TextureCubemap * {
   if (!s_CubemapRenderpass) {
      TextureCubemapVk::InitResources();
   }

   static std::unordered_map<TextureCubemapVk *, std::shared_ptr<TextureCubemap>> loadedTextures;
   auto it = loadedTextures.find(this);
   if (it != loadedTextures.end()) {
      return it->second.get();
   }

   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   auto irradianceMap = std::make_unique<TextureCubemapVk>(resolution, resolution, 1);

   const size_t cubeFaceCount = 6;
   const uint32_t cubeVertexCount = 36;
   VkExtent2D faceExtent{resolution, resolution};

   // Depth image
   vk::Image depthImage(device, {device.GfxQueueIdx()},
                        faceExtent, 1,
                        VK_SAMPLE_COUNT_1_BIT,
                        device.findDepthFormat(),
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

   vk::DeviceMemory depthImageMemory(device, device, {&depthImage}, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   depthImage.BindMemory(depthImageMemory.data(), 0);
   vk::ImageView depthImageView(device, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);


   // Destination Irradiance resources
   irradianceMap->m_TextureMemory = device.allocateImageMemory({irradianceMap->m_TextureImage},
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   irradianceMap->m_TextureImage->BindMemory(irradianceMap->m_TextureMemory->data(), 0);
   irradianceMap->m_TextureView = device.createImageView(*irradianceMap->m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);

   VkImageViewCreateInfo faceSubViewCreateInfo = {};
   faceSubViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   faceSubViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   faceSubViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
   faceSubViewCreateInfo.image = irradianceMap->m_TextureImage->data();
   faceSubViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   faceSubViewCreateInfo.subresourceRange.baseMipLevel = 0;
   faceSubViewCreateInfo.subresourceRange.levelCount = 1;
   faceSubViewCreateInfo.subresourceRange.layerCount = 1;

   std::vector<ImageView> faceSubviews;
   std::vector<vk::Framebuffer> faceFramebuffers;
   for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
      faceSubViewCreateInfo.subresourceRange.baseArrayLayer = faceIdx;
      faceSubviews.emplace_back(device, faceSubViewCreateInfo);
      faceFramebuffers.emplace_back(device, s_CubemapRenderpass->data(), faceExtent,
                                    std::vector{faceSubviews.back().data(), depthImageView.data()});
   }

   // Bind previously created environment map as a source texture for irradiance computation
   s_IrradiancePipeline->BindCubemaps({this}, BindingKey(0, 0));

   VkViewport viewport{
           0.0f,
           0.0f,
           static_cast<float>(resolution),
           static_cast<float>(resolution),
           0.0f,
           1.0f
   };
   VkRect2D scissor{};
   scissor.extent = faceExtent;


   CommandPool pool(device, device.GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];

   // Render part of the HDR texture to each face of the cubemap
   for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
      setupCmdBuffer.Begin();

      s_CubemapRenderpass->Begin(setupCmdBuffer, faceFramebuffers[faceIdx]);
      s_IrradiancePipeline->Bind(setupCmdBuffer.data());
      s_IrradiancePipeline->BindDescriptorSets(0, {});

      vkCmdSetViewport(setupCmdBuffer.data(), 0, 1, &viewport);
      vkCmdSetScissor(setupCmdBuffer.data(), 0, 1, &scissor);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(setupCmdBuffer.data(), 0, 1, s_CubeVertexBuffer->ptr(), &offset);

      s_IrradiancePipeline->PushConstants(setupCmdBuffer.data(),
                                          {VK_SHADER_STAGE_VERTEX_BIT, 0}, s_CaptureViews[faceIdx]);
      vkCmdDraw(setupCmdBuffer.data(), cubeVertexCount, 1, 0, 0);

      s_CubemapRenderpass->End(setupCmdBuffer);

      setupCmdBuffer.End();
      setupCmdBuffer.Submit(device.GfxQueue());
      vkQueueWaitIdle(device.GfxQueue());
      vkResetCommandBuffer(setupCmdBuffer.data(), 0);
   }

//    setupCmdBuffer.End();
//    setupCmdBuffer.Submit(device.GfxQueue());
//    vkQueueWaitIdle(device.GfxQueue());

   it = loadedTextures.emplace(this, std::move(irradianceMap)).first;
   return it->second.get();
}

auto TextureCubemapVk::CreatePrefilteredCubemap(uint32_t baseResolution, uint32_t maxMipLevels) -> TextureCubemap * {
   static std::unordered_map<TextureCubemapVk *, std::shared_ptr<TextureCubemap>> loadedTextures;
   auto it = loadedTextures.find(this);
   if (it != loadedTextures.end()) {
      return it->second.get();
   }

   auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
   Device &device = gfxContext.GetDevice();

   auto prefilteredMap = std::make_unique<TextureCubemapVk>(baseResolution, baseResolution, maxMipLevels);
   uint32_t faceMipLevels = std::min(prefilteredMap->m_TextureImage->Info().mipLevels, maxMipLevels);

   const size_t cubeFaceCount = 6;
   const uint32_t cubeVertexCount = 36;
   VkExtent2D faceExtent{baseResolution, baseResolution};

   VkFormat depthFormat = device.findDepthFormat();

   // Depth image
   vk::Image depthImage(device, {device.GfxQueueIdx()},
                        faceExtent, faceMipLevels,
                        VK_SAMPLE_COUNT_1_BIT,
                        depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

   vk::DeviceMemory depthImageMemory(device, device, {&depthImage}, {}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   depthImage.BindMemory(depthImageMemory.data(), 0);
//    vk::ImageView depthImageView(device, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

   // Destination Irradiance resources
   prefilteredMap->m_TextureMemory = device.allocateImageMemory({prefilteredMap->m_TextureImage},
                                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   prefilteredMap->m_TextureImage->BindMemory(prefilteredMap->m_TextureMemory->data(), 0);
   prefilteredMap->m_TextureView = device.createImageView(*prefilteredMap->m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);


   VkImageViewCreateInfo depthSubViewCreateInfo = {};
   depthSubViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   depthSubViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   depthSubViewCreateInfo.format = depthFormat;
   depthSubViewCreateInfo.image = depthImage.data();
   depthSubViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
   depthSubViewCreateInfo.subresourceRange.baseArrayLayer = 0;
   depthSubViewCreateInfo.subresourceRange.levelCount = 1;
   depthSubViewCreateInfo.subresourceRange.layerCount = 1;

   VkImageViewCreateInfo faceSubViewCreateInfo = {};
   faceSubViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   faceSubViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   faceSubViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
   faceSubViewCreateInfo.image = prefilteredMap->m_TextureImage->data();
   faceSubViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   faceSubViewCreateInfo.subresourceRange.levelCount = 1;
   faceSubViewCreateInfo.subresourceRange.layerCount = 1;

   // Bind previously created environment map as a source texture for irradiance computation
   s_PrefilterPipeline->BindCubemaps({this}, BindingKey(0, 0));

   VkViewport viewport{
           0.0f,
           0.0f,
           static_cast<float>(baseResolution),
           static_cast<float>(baseResolution),
           0.0f,
           1.0f
   };
   VkRect2D scissor{};
   scissor.extent = faceExtent;

   CommandPool pool(device, device.GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   CommandBuffers setupCmdBuffers(device, pool.data());
   CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
   setupCmdBuffer.Begin();

   std::vector<ImageView> depthSubViews;
   std::vector<std::vector<ImageView>> subviews(faceMipLevels);
   std::vector<std::vector<vk::Framebuffer>> framebuffers(faceMipLevels);

   VkExtent2D mipExtent = faceExtent;
   for (uint32_t mipLevel = 0; mipLevel < faceMipLevels; mipLevel++) {
      faceSubViewCreateInfo.subresourceRange.baseMipLevel = mipLevel;
      depthSubViewCreateInfo.subresourceRange.baseMipLevel = mipLevel;

      std::vector<ImageView> &faceSubviews = subviews[mipLevel];
      std::vector<vk::Framebuffer> &faceFramebuffers = framebuffers[mipLevel];
      depthSubViews.emplace_back(device, depthSubViewCreateInfo);

      for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
         faceSubViewCreateInfo.subresourceRange.baseArrayLayer = faceIdx;
         faceSubviews.emplace_back(device, faceSubViewCreateInfo);
         faceFramebuffers.emplace_back(device, s_CubemapRenderpass->data(), mipExtent,
                                       std::vector{faceSubviews.back().data(), depthSubViews.back().data()});
      }
      viewport.width = mipExtent.width;
      viewport.height = mipExtent.height;
      scissor.extent = mipExtent;

      float roughness = (float) mipLevel / (float) (faceMipLevels - 1);

      // Render part of the HDR texture to each face of the cubemap
      for (size_t faceIdx = 0; faceIdx < cubeFaceCount; faceIdx++) {
         s_CubemapRenderpass->Begin(setupCmdBuffer, faceFramebuffers[faceIdx]);
         s_PrefilterPipeline->Bind(setupCmdBuffer.data());
         s_PrefilterPipeline->BindDescriptorSets(0, {});

         vkCmdSetViewport(setupCmdBuffer.data(), 0, 1, &viewport);
         vkCmdSetScissor(setupCmdBuffer.data(), 0, 1, &scissor);

         VkDeviceSize offset = 0;
         vkCmdBindVertexBuffers(setupCmdBuffer.data(), 0, 1, s_CubeVertexBuffer->ptr(), &offset);

         s_PrefilterPipeline->PushConstants(setupCmdBuffer.data(), {VK_SHADER_STAGE_VERTEX_BIT, 0},
                                            s_CaptureViews[faceIdx]);
         s_PrefilterPipeline->PushConstants(setupCmdBuffer.data(), {VK_SHADER_STAGE_FRAGMENT_BIT, 1}, roughness);
         s_PrefilterPipeline->PushConstants(setupCmdBuffer.data(), {VK_SHADER_STAGE_FRAGMENT_BIT, 2},
                                            baseResolution);

         vkCmdDraw(setupCmdBuffer.data(), cubeVertexCount, 1, 0, 0);

         s_CubemapRenderpass->End(setupCmdBuffer);
      }


      mipExtent.width *= 0.5f;
      mipExtent.height *= 0.5f;
   }

   setupCmdBuffer.End();
   setupCmdBuffer.Submit(device.GfxQueue());
   vkQueueWaitIdle(device.GfxQueue());

   it = loadedTextures.emplace(this, std::move(prefilteredMap)).first;
   return it->second.get();
}


bool g_TextureResourcesInitialized = false;

void InitializeTextureResources() {
   if (!g_TextureResourcesInitialized) {
      Texture2DVk::InitResources();
      TextureCubemapVk::InitResources();
      g_TextureResourcesInitialized = true;
   }
}

void ReleaseTextureResources() {
   if (g_TextureResourcesInitialized) {
      Texture2DVk::ReleaseResources();
      TextureCubemapVk::ReleaseResources();
      g_TextureResourcesInitialized = false;
   }
}
