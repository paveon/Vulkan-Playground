#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>

struct UniformBufferObject {
    alignas(16) math::mat4 model;
    alignas(16) math::mat4 view;
    alignas(16) math::mat4 proj;
};

class TestLayer : public Layer {
    const char *MODEL_PATH = BASE_DIR "/models/chalet.obj";
    const char *TEXTURE_PATH = BASE_DIR "/textures/chalet.jpg";

    const char *vertShaderPath = "shaders/triangle.vert.spv";
    const char *fragShaderPath = "shaders/triangle.frag.spv";

    GfxContextVk &m_Context;
    Device &m_Device;
    Renderer &m_Renderer;

    vk::CommandBuffers m_CmdBuffers;

    std::unique_ptr<RenderPass> m_RenderPass;
    std::unique_ptr<Pipeline> m_GraphicsPipeline;

    std::unique_ptr<Model> m_Model;
    std::unique_ptr<VertexBuffer> m_VertexBuffer;
    std::unique_ptr<IndexBuffer> m_IndexBuffer;
    std::unique_ptr<Texture2D> m_Texture;
    std::unique_ptr<UniformBuffer> m_UniformBuffer;

    VkRenderPassBeginInfo m_RenderPassBeginInfo = {};
    UniformBufferObject m_UBO = {};

    PerspectiveCamera m_Camera;

    float m_MouseSensitivity = 0.1f;
    float m_MoveSpeed = 3.0f;

public:
    explicit TestLayer(const char *name, Renderer &renderer) :
            Layer(name),
            m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
            m_Device(m_Context.GetDevice()),
            m_Renderer(renderer) {

        auto[width, height] = m_Renderer.FramebufferSize();
        m_Camera = PerspectiveCamera(
                math::vec3(0.0f, 3.0f, 0.0f),
                math::vec3(0.0f, -1.0f, 0.0f),
                width / (float) height,
                0.1f, 10.0f, math::radians(45.0f));

        m_UBO.proj = m_Camera.GetProjection();
        m_UBO.view = m_Camera.GetView();
        m_UBO.model = math::mat4(1.0f);

        m_RenderPass = RenderPass::Create();
    }

    void OnAttach() override {
        Layer::OnAttach();

        m_CmdBuffers = vk::CommandBuffers(m_Device,
                                          m_Device.GfxPool()->data(),
                                          VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                          m_Context.Swapchain().ImageCount());

        std::unique_ptr<ShaderProgram> fragShader(ShaderProgram::Create(fragShaderPath));
        std::unique_ptr<ShaderProgram> vertexShader(ShaderProgram::Create(vertShaderPath));
        VertexLayout vertexLayout{
                VertexAttribute(ShaderType::Float3),
                VertexAttribute(ShaderType::Float3),
                VertexAttribute(ShaderType::Float2)
        };

        DescriptorLayout descriptorLayout{
                DescriptorBinding(DescriptorType::UniformBuffer),
                DescriptorBinding(DescriptorType::Texture)
        };

        m_GraphicsPipeline = Pipeline::Create(*m_RenderPass, *vertexShader, *fragShader, vertexLayout, descriptorLayout,
                                              {},
                                              true);

        /* Create vertex and index buffers */
        m_Model = std::make_unique<Model>(MODEL_PATH);
        m_VertexBuffer = VertexBuffer::Create();
        m_IndexBuffer = IndexBuffer::Create();
        m_VertexBuffer->UploadData(m_Model->Vertices(), m_Model->VertexDataSize());
        m_IndexBuffer->UploadData(m_Model->Indices(), m_Model->IndexDataSize());

        m_Texture = Texture2D::Create(TEXTURE_PATH);
        m_Texture->Upload();

        m_UniformBuffer = UniformBuffer::Create(sizeof(UniformBufferObject));
        m_GraphicsPipeline->BindUniformBuffer(*m_UniformBuffer, 0);
        m_GraphicsPipeline->BindTexture(*m_Texture, 1);
    }

    void OnDetach() override {
        Layer::OnDetach();
    }

    auto OnWindowResize(WindowResizeEvent &e) -> bool override {
        m_RenderPass = RenderPass::Create();

        m_CmdBuffers = vk::CommandBuffers(m_Device,
                                          m_Device.GfxPool()->data(),
                                          VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                          m_Context.Swapchain().ImageCount());

        m_UniformBuffer = UniformBuffer::Create(sizeof(UniformBufferObject));

        m_GraphicsPipeline->Recreate(*m_RenderPass);
        m_GraphicsPipeline->BindUniformBuffer(*m_UniformBuffer, 0);
        m_GraphicsPipeline->BindTexture(*m_Texture, 1);

        m_Camera.SetAspectRatio(e.Width() / (float) e.Height());
        m_UBO.proj = m_Camera.GetProjection();
        return true;
    }

    auto OnKeyPress(KeyPressEvent &) -> bool override {
//        if (e.RepeatCount() < 1) std::cout << m_DebugName << "::" << e << std::endl;
        return true;
    }

    auto OnKeyRelease(KeyReleaseEvent &) -> bool override {
//        std::cout << m_DebugName << "::" << e << std::endl;
        return true;
    }

    auto OnMouseMove(MouseMoveEvent &) -> bool override {
        return true;
    }

    void OnUpdate(Timestep ts, uint32_t imageIndex) override {
        static auto lastMousePos = Input::MousePos();

        auto mousePos = Input::MousePos();

        if (mousePos != lastMousePos) {
            if (Input::MouseButtonPressed(IO_MOUSE_BUTTON_RIGHT)) {
                float deltaX = (lastMousePos.first - mousePos.first) * m_MouseSensitivity * ts;
                float deltaY = (lastMousePos.second - mousePos.second) * m_MouseSensitivity * ts;
                m_Camera.ChangeYawPitch(deltaX, deltaY);
                m_UBO.view = m_Camera.GetView();
            }
            lastMousePos = mousePos;
        }


        if (Input::KeyPressed(IO_KEY_E)) {
            m_Camera.MoveZ(m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        } else if (Input::KeyPressed(IO_KEY_Q)) {
            m_Camera.MoveZ(-m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        }

        if (Input::KeyPressed(IO_KEY_W)) {
            m_Camera.MoveInDirection(m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        } else if (Input::KeyPressed(IO_KEY_S)) {
            m_Camera.MoveInDirection(-m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        }

        if (Input::KeyPressed(IO_KEY_A)) {
            m_Camera.MoveSideways(-m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        } else if (Input::KeyPressed(IO_KEY_D)) {
            m_Camera.MoveSideways(m_MoveSpeed * ts);
            m_UBO.view = m_Camera.GetView();
        }


//        m_UBO.model = math::rotate(math::mat4(1.0f), time * math::radians(15.0f), math::vec3(0.0f, 0.0f, 1.0f));

//        auto[width, height] = Application::GetGraphicsContext().FramebufferSize();

        m_UniformBuffer->SetData(imageIndex, &m_UBO);
    }

    auto OnDraw(uint32_t imageIndex, const VkCommandBufferInheritanceInfo &info) -> VkCommandBuffer override {
        auto[width, height] = Application::GetGraphicsContext().FramebufferSize();
        VkExtent2D extent{width, height};

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        VkClearColorValue colorClear = {{0.0f, 0.0f, 0.0f, 1.0f}};
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = colorClear;
        clearValues[1].depthStencil = {1.0f, 0};

//        m_RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//        m_RenderPassBeginInfo.renderPass = (VkRenderPass) m_RenderPass->VkHandle();
//        m_RenderPassBeginInfo.renderArea.offset = {0, 0};
//        m_RenderPassBeginInfo.clearValueCount = clearValues.size();
//        m_RenderPassBeginInfo.pClearValues = clearValues.data();
//        m_RenderPassBeginInfo.renderArea.extent = extent;
//        m_RenderPassBeginInfo.framebuffer = m_Renderer.GetFramebuffer().data();

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        beginInfo.pInheritanceInfo = &info;

        vk::CommandBuffer cmdBuffer(m_CmdBuffers[imageIndex]);
        cmdBuffer.Begin(beginInfo);

//        vkCmdBeginRenderPass(cmdBuffer.data(), &m_RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(cmdBuffer.data(), 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer.data(), 0, 1, &scissor);

        m_GraphicsPipeline->Bind(cmdBuffer.data(), imageIndex);

        m_VertexBuffer->Bind(cmdBuffer.data(), 0);
        m_IndexBuffer->Bind(cmdBuffer.data(), 0);

        vkCmdDrawIndexed(cmdBuffer.data(), m_Model->IndexCount(), 1, 0, 0, 0);

//        vkCmdEndRenderPass(cmdBuffer.data());

        cmdBuffer.End();

        return cmdBuffer.data();
    }
};


class SandboxApp : public Application {
public:
    SandboxApp() : Application("Sandbox Application") {
        PushLayer(std::make_unique<TestLayer>("TestLayer", *m_Renderer));
        ImGuiLayer *imGui = static_cast<ImGuiLayer *>(PushOverlay(ImGuiLayer::Create(*m_Renderer)));

        imGui->SetDrawCallback([]() {
            ImGui::Begin("Test window");
            static float rotation = 0.0;
            ImGui::SliderFloat("rotation", &rotation, 0.0f, 360.0f);
            static float translation[] = {0.0, 0.0};
            ImGui::SliderFloat2("position", translation, -1.0, 1.0);
            static float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            ImGui::ColorEdit3("color", color);
            ImGui::End();
        });
    }

    ~SandboxApp() override = default;
};


auto Application::CreateApplication() -> std::unique_ptr<Application> { return std::make_unique<SandboxApp>(); }