#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>
#include <Platform/Vulkan/GraphicsContextVk.h>
#include <glm/matrix.hpp>
//#include <assimp>

struct DirectionalLight {
    glm::vec4 direction;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

struct PointLight {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 coeffs;
};

struct SpotLight {
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 cutOffs;
};

struct SceneUBO {
    glm::vec4 cameraPosition;
    DirectionalLight light;
    SpotLight spotLight;
    int32_t pointLightCount;
};


struct ModelUBO {
    glm::mat4 mvp;
};


class TestLayer : public Layer {
    const char *BACKPACK_MODEL_ASSET_PATH = BASE_DIR "/models/backpack.obj";
    const char *MODEL_PATH = BASE_DIR "/models/chalet.obj";
    const char *CHALET_TEXTURE_PATH = BASE_DIR "/textures/chalet.jpg";
    const char *CONTAINER_DIFFUSE_TEX_PATH = BASE_DIR "/textures/container_diffuse.png";
    const char *CONTAINER_SPECULAR_TEX_PATH = BASE_DIR "/textures/container_specular.png";
    const char *CONTAINER_EMISSION_TEX_PATH = BASE_DIR "/textures/matrix_emission_map.jpg";

//    const char *vertShaderPath = "shaders/triangle.vert.spv";
//    const char *fragShaderPath = "shaders/triangle.frag.spv";

    const char *phongVertShaderPath = "shaders/cube.vert.spv";
    const char *phongFragShaderPath = "shaders/cube.frag.spv";

    const char *vertLightShader = "shaders/lightCube.vert.spv";
    const char *fragLightShader = "shaders/lightCube.frag.spv";

    GfxContextVk &m_Context;
    Device &m_Device;

//    std::unique_ptr<VertexBuffer> m_VertexBuffer;
//    std::unique_ptr<IndexBuffer> m_IndexBuffer;
    std::vector<std::shared_ptr<Texture2D>> m_Textures;
//    std::vector<std::unique_ptr<UniformBuffer>> m_UniformBuffers;

    std::vector<Material *> m_AssetMaterials;
//    std::vector<std::shared_ptr<Material>> m_Materials;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
//    std::vector<Model> m_ChaletModels;
//    std::vector<Model> m_CubeModels;
//    std::vector<Model> m_PointLightModels;

    std::vector<std::shared_ptr<ShaderPipeline>> m_Shaders;
    std::vector<std::unique_ptr<ModelAsset>> m_ModelAssets;
    std::vector<Entity> m_Entities;

//    std::vector<Model> m_TestModels;
//    ModelInstance m_TestModelInstance;

    std::vector<glm::vec4> m_LightPositions{
            glm::vec4(2.0f, 0.0f, -0.5f, 1.0f),
            glm::vec4(1.0f, 1.0f, -0.5f, 1.0f),
            glm::vec4(0.0f, 1.0f, -0.5f, 1.0f),
            glm::vec4(-1.0f, 1.0f, -0.5f, 1.0f),
            glm::vec4(-2.0f, 1.0f, -0.5f, 1.0f),
    };
    std::vector<PointLight> m_PointLights;


    SceneUBO m_SceneUBO{
            glm::vec4(0.0f),
            DirectionalLight{
                    glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
                    glm::vec4(0.2f, 0.2f, 0.2f, 0.0f),
                    glm::vec4(0.75f, 0.75f, 0.75f, 0.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
            },
            SpotLight{
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                    glm::vec4(0.2f, 0.2f, 0.2f, 0.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                    glm::vec4(std::cos(glm::radians(6.25f)), std::cos(glm::radians(9.375f)), 0.0f, 0.0f)
            },
            0,
    };


    std::unique_ptr<UniformBuffer> m_SceneUB;
    std::unique_ptr<UniformBuffer> m_LightsUB;

    std::shared_ptr<PerspectiveCamera> m_Camera;

    float m_LightRotation = 0.0f;
    float m_MouseSensitivity = 0.2f;
    float m_MoveSpeed = 3.0f;

public:
    explicit TestLayer(const char *name) :
            Layer(name),
            m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
            m_Device(m_Context.GetDevice()),
            m_PointLights(m_LightPositions.size(), {
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.05f, 0.05f, 0.05f, 0.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 0.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                    glm::vec4(1.0f, 0.09f, 0.032f, 0.0f)
            }) {


        auto[width, height] = m_Context.Swapchain().Extent();
        m_Camera = std::make_shared<PerspectiveCamera>(
                glm::vec3(0.0f, 3.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f),
                width / (float) height,
                0.01f, 200.0f, glm::radians(45.0f));

//        m_UBO.proj = m_Camera->GetProjection();
//        m_UBO.view = m_Camera->GetView();
//        m_UBO.model = glm::mat4(1.0f);
//        m_UBO.mvp = m_Camera->GetProjection() * m_Camera->GetView() * glm::mat4(1.0f);

//        m_RenderPass = RenderPass::Create();
    }

    void OnAttach(const LayerStack *stack) override {
        Layer::OnAttach(stack);

        auto[width, height] = Application::GetGraphicsContext().FramebufferSize();
//        Renderer::SubmitCommand(RenderCommand::SetClearColor({0.64f, 0.84f, 0.91f, 1.0f}));
        Renderer::SubmitCommand(RenderCommand::SetClearColor({0.0f, 0.0f, 0.0f, 1.0f}));
        Renderer::SubmitCommand(RenderCommand::SetViewport(Viewport{
                0.0f,
                static_cast<float>(height),
                static_cast<float>(width),
                -static_cast<float>(height),
                0.0f,
                1.0f
        }));
        Renderer::SubmitCommand(RenderCommand::SetScissor(Scissor{
                0,
                0,
                width,
                height
        }));

//        std::vector<std::pair<const char *, ShaderType>> shaderStages{
//                {vertShaderPath, ShaderType::VERTEX_SHADER},
//                {fragShaderPath, ShaderType::FRAGMENT_SHADER},
//        };

//        m_Shaders.emplace_back(ShaderPipeline::Create(shaderStages, {{1, 0},
//                                                                     {2, 0}},
//                                                      Renderer::GetRenderPass(), {}, true));

        std::vector<std::pair<const char *, ShaderType>> shaderStages{
                {phongVertShaderPath, ShaderType::VERTEX_SHADER},
                {phongFragShaderPath, ShaderType::FRAGMENT_SHADER}
        };

        m_Shaders.emplace_back(ShaderPipeline::Create("PhongShader",
                                                      shaderStages,
                                                      {{0, 0},
                                                       {4, 0}},
                                                      Renderer::GetRenderPass(), {}, true));

//        shaderStages[0] = {vertLightShader, ShaderType::VERTEX_SHADER};
//        shaderStages[1] = {fragLightShader, ShaderType::FRAGMENT_SHADER};
//        m_Shaders.emplace_back(ShaderPipeline::Create("Light Shader", shaderStages, {{0, 0}},
//                                                      Renderer::GetRenderPass(), {}, true));

        m_Textures.emplace_back(Texture2D::Create(CHALET_TEXTURE_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_DIFFUSE_TEX_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_SPECULAR_TEX_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_EMISSION_TEX_PATH));
        m_Textures[0]->Upload();
        m_Textures[1]->Upload();
        m_Textures[2]->Upload();
        m_Textures[3]->Upload();

//        m_Materials.emplace_back(std::make_shared<Material>("chaletMaterial", m_Shaders[0]));

//        m_Materials.emplace_back(std::make_shared<Material>("cubeMaterial", m_Shaders[0]));
//        m_Materials.emplace_back(std::make_shared<Material>("lightMaterial", m_Shaders[1]));

//        m_Materials[0]->AllocateResources(10);
//        m_Materials[1]->AllocateResources(100);
//        m_Materials[2]->AllocateResources(10);

//        auto indicesMat0 = m_Materials[0]->BindTextures({m_Textures[0].get()}, 3, 0);
//        auto indicesMat1 = m_Materials[1]->BindTextures({m_Textures[1].get(), m_Textures[2].get(), m_Textures[3].get()},
//                                                        3, 0);

        m_ModelAssets.emplace_back(ModelAsset::LoadModel(BACKPACK_MODEL_ASSET_PATH));
        auto &backpackAsset = m_ModelAssets.back();
        auto &backpackMaterials = backpackAsset->GetMaterials();
        backpackAsset->StageMeshes();
        for (auto &material : backpackMaterials) {
            m_AssetMaterials.push_back(&material);
            material.SetShaderPipeline(m_Shaders[0], BindingKey(4, 0));
            material.BindTextures(backpackAsset->TextureVector(), BindingKey(1, 0));
        }

        m_Entities.emplace_back(m_ModelAssets.back());
        Entity::AllocateTransformsUB(256);
        m_Entities.back().SetScale(glm::vec3(0.5f));
        m_Entities.back().SetRotationX(HALF_PI_F);
        m_Entities.back().SetRotationY(PI_F);

//        m_TestModels.emplace_back(Model(TEST_MODEL_PATH));
//        m_TestModels.back().StageMeshData();
//        m_TestModels.back().SetMaterial(m_Materials[1].get(), {3, 0},{1, 0});
//        m_TestModelInstance = m_TestModels.back().CreateInstance();
//        m_TestModelInstance.SetPosition(glm::vec3(0.0f));
//        m_TestModelInstance.SetScale(glm::vec3(0.5f));
//        m_TestModelInstance.SetRotationX(HALF_PI_F);

//        m_SceneUBO.pointLightCount = m_PointLights.size();
        m_SceneUBO.pointLightCount = 0;
        m_SceneUBO.cameraPosition = glm::vec4(m_Camera->GetPosition(), 1.0f);

        size_t ubSize = sizeof(PointLight) * m_PointLights.size();
        m_LightsUB = UniformBuffer::Create("Lights UB", ubSize, 1);
        m_SceneUB = UniformBuffer::Create("Scene UB", sizeof(SceneUBO), 1);

        m_LightsUB->SetData(m_PointLights.data(), 1, 0);
        m_SceneUB->SetData(&m_SceneUBO, 1, 0);

//        m_Shaders[1]->BindUniformBuffer(Entity::TransformsUB(), 0, 0);

//        m_Materials[1]->BindUniformBuffer(*m_SceneUB, 2, 0);
//        m_Materials[2]->BindUniformBuffer(*m_SceneUB, 0, 0);
//        m_Materials[0]->BindUniformBuffer(*m_LightsUB, 4, 0);



//        m_Materials[0]->SetUniform(2, 0, m_SceneUBO);
//        m_Materials[1]->SetUniform(0, 0, m_SceneUBO);


//        m_Meshes.emplace_back(Mesh::FromOBJ(MODEL_PATH));
//        m_Meshes.emplace_back(Mesh::Cube());

//        MaterialUBO materialSpec{
//                glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
//                glm::vec4(1.0f),
//                glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
//                32.0f,
//                (uint32_t)(indicesMat1[0]),
//                (uint32_t)(indicesMat1[1]),
//                (uint32_t)(indicesMat1[2])
//        };

//        m_ChaletModels.emplace_back();
//        m_ChaletModels.back().AttachMesh(m_Meshes[0]);
//        m_ChaletModels.back().SetMaterial(m_Materials[0].get(), std::pair<uint32_t, uint32_t>(),
//                                          std::pair<uint32_t, uint32_t>());
//        m_ChaletModels.back().SetScale(glm::vec3(0.5f));
//        m_ChaletModels.back().SetUniform(1, 0, materialSpec);

//        m_ChaletModels.emplace_back();
//        m_ChaletModels.back().AttachMesh(m_Meshes[0]);
//        m_ChaletModels.back().SetMaterial(m_Materials[0].get(), std::pair<uint32_t, uint32_t>(),
//                                          std::pair<uint32_t, uint32_t>());
//        m_ChaletModels.back().SetPosition(glm::vec3(1.0f));
//        m_ChaletModels.back().SetUniform(1, 0, materialSpec);

//        m_CubeModels.emplace_back();
//        m_CubeModels.back().AttachMesh(m_Meshes[1]);
//        m_CubeModels.back().SetMaterial(m_Materials[1].get(), std::pair<uint32_t, uint32_t>(),
//                                        std::pair<uint32_t, uint32_t>());
//        m_CubeModels.back().SetPosition(glm::vec3(0.0f, 0.0f, -1.0f));
//        m_CubeModels.back().SetScale(glm::vec3(0.75f));
//        m_CubeModels.back().SetColor(glm::vec3(1.0f, 0.5f, 0.31f));
//        m_CubeModels.back().SetUniform(1, 0, materialSpec);

//        glm::mat4 viewModel = m_Camera->GetView() * m_CubeModels.back().GetModelMatrix();
//        glm::mat4 mvp = m_Camera->GetProjection() * viewModel;
//        glm::mat4 normalMatrix = m_CubeModels.back().GetNormalMatrix() * m_Camera->GetView();
//        TransformUBO ubo{
//                mvp,
//                viewModel,
//                normalMatrix,
//                m_CubeModels.back().GetColor(),
//                0
//        };
//        m_CubeModels.back().SetUniform(2, 0, ubo);

//        for (const auto &chalet : m_ChaletModels) {
//            ubo.viewModel = m_Camera->GetView() * chalet.GetModelMatrix();
//            ubo.mvp = m_Camera->GetProjection() * ubo.viewModel;
//            ubo.normalMatrix = chalet.GetNormalMatrix() * m_Camera->GetView();
//            ubo.objectColor = glm::vec3(0.0f);
//            chalet.SetUniform(2, 0, ubo);
//        }

//        glm::mat4 projectionView(m_Camera->GetProjectionView());
//        for (const auto &pos : m_LightPositions) {
//            m_PointLightModels.emplace_back();
//            auto &light = m_PointLightModels.back();
////            light.AttachMesh(m_Meshes[1]);
//            light.SetMaterial(m_Materials[2].get(), std::pair<uint32_t, uint32_t>(), std::pair<uint32_t, uint32_t>());
//            light.SetPosition(pos);
//            light.SetScale(glm::vec3(0.5f));
//            light.SetUniform(0, 0, ModelUBO{projectionView * light.GetModelMatrix()});
//        }

//        for (const auto &material : m_Materials)
//            material->UpdateUniforms();

//        for (const auto &mesh : m_Meshes)
//            mesh->StageData();

//        for (const auto& model : m_TestModels) {
//            model.StageMeshData();
//        }

        m_Shaders[0]->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
        m_Shaders[0]->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
        m_AssetMaterials[0]->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
        m_AssetMaterials[0]->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
        Entity::UpdateTransformsUB(*m_Camera);

        for (auto &shader : m_Shaders) {
            shader->AllocateResources();
        }

        Renderer::FlushStagedData();
    }

    void OnDetach() override {
        Layer::OnDetach();
    }

    auto OnWindowResize(WindowResizeEvent &e) -> bool override {
//        m_UniformBuffers[0] = UniformBuffer::Create(sizeof(ModelUBO), m_ModelUBOs.size());
//        m_UniformBuffers[1] = UniformBuffer::Create(sizeof(TransformUBO), m_ModelUBOs.size());
//        m_Materials[0]->BindUniformBuffer(m_UniformBuffers[0], 0, 0);
//        m_Materials[1]->BindUniformBuffer(m_UniformBuffers[1], 0, 0);

        m_Camera->SetAspectRatio(e.Width() / (float) e.Height());

//        glm::mat4 projectionView(m_Camera->GetProjection() * m_Camera->GetView());
//        for (size_t i = 0; i < m_ModelUBOs.size(); i++) {
//            m_ModelUBOs[i].mvp = projectionView * m_Models[i].GetModelMatrix();
//        }

        Renderer::SubmitCommand(RenderCommand::SetViewport(Viewport{
                0.0f,
                static_cast<float>(e.Height()),
                static_cast<float>(e.Width()),
                -static_cast<float>(e.Height()),
                0.0f,
                1.0f
        }));
        Renderer::SubmitCommand(RenderCommand::SetScissor(Scissor{
                0,
                0,
                e.Width(),
                e.Height()
        }));
        return true;
    }

    auto OnKeyPress(KeyPressEvent &) -> bool override {
        return true;
    }

    auto OnKeyRelease(KeyReleaseEvent &) -> bool override {
        return true;
    }

    auto OnMouseMove(MouseMoveEvent &) -> bool override {
        return true;
    }

    void OnUpdate(Timestep ts) override {
        static float time = 0.0f;
        static auto lastMousePos = Input::MousePos();

        auto mousePos = Input::MousePos();

        if (mousePos != lastMousePos) {
            if (Input::MouseButtonPressed(IO_MOUSE_BUTTON_RIGHT)) {
                float deltaX = (lastMousePos.first - mousePos.first) * m_MouseSensitivity * ts;
                float deltaY = (lastMousePos.second - mousePos.second) * m_MouseSensitivity * ts;
                m_Camera->ChangeYawPitch(deltaX, deltaY);
            }
            lastMousePos = mousePos;
        }


        if (Input::KeyPressed(IO_KEY_E)) {
            m_Camera->MoveZ(m_MoveSpeed * ts);
        } else if (Input::KeyPressed(IO_KEY_Q)) {
            m_Camera->MoveZ(-m_MoveSpeed * ts);
        }

        if (Input::KeyPressed(IO_KEY_W)) {
            m_Camera->MoveInDirection(m_MoveSpeed * ts);
        } else if (Input::KeyPressed(IO_KEY_S)) {
            m_Camera->MoveInDirection(-m_MoveSpeed * ts);
        }

        if (Input::KeyPressed(IO_KEY_A)) {
            m_Camera->MoveSideways(-m_MoveSpeed * ts);
        } else if (Input::KeyPressed(IO_KEY_D)) {
            m_Camera->MoveSideways(m_MoveSpeed * ts);
        }

//        time += ts.GetSeconds();
//        const float radius = 2.0f;
//        const float x = (m_LightRotation / 360.0f) * TWO_PI_F;
//        float lightX = std::sin(x) * radius;
//        float lightY = std::cos(x) * radius;
//        float lightZ = std::sin(m_Time) * 0.5 - 1.0f;
//        m_SceneUBO.lightPosition = glm::vec4(lightX, lightY, -1.0f, 1.0f);
        m_SceneUBO.cameraPosition = glm::vec4(m_Camera->GetPosition(), 1.0f);

        SceneUBO sceneUBO = m_SceneUBO;
        sceneUBO.light.direction = m_Camera->GetView() * m_SceneUBO.light.direction;
        sceneUBO.spotLight.position = m_Camera->GetView() * glm::vec4(m_Camera->GetPosition(), 1.0f);
        sceneUBO.spotLight.direction = m_Camera->GetView() * glm::vec4(m_Camera->GetDirection(), 0.0f);

//        m_Materials[1]->SetUniform(0, 0, sceneUBO);
        m_SceneUB->SetData(&sceneUBO, 1, 0);

//        glm::mat4 viewModel = m_Camera->GetView() * m_CubeModels[0].GetModelMatrix();
//        glm::mat4 mvp = m_Camera->GetProjection() * viewModel;
//        glm::mat4 normalMatrix = m_CubeModels[0].GetNormalMatrix() * m_Camera->GetView();
//        TransformUBO ubo{mvp,
//                    viewModel,
//                    normalMatrix,
//                    m_CubeModels[0].GetColor(),
//                    0
//        };
//        m_CubeModels[0].SetUniform(2, 0, ubo);

//        for (const auto &chalet : m_ChaletModels) {
//            ubo.viewModel = m_Camera->GetView() * chalet.GetModelMatrix();
//            ubo.mvp = m_Camera->GetProjection() * ubo.viewModel;
//            ubo.normalMatrix = chalet.GetNormalMatrix() * m_Camera->GetView();
//            ubo.objectColor = glm::vec3(0.0f);
//            ubo.materialIdx = 0;
//            chalet.SetUniform(2, 0, ubo);
//        }

//        for (size_t i = 0; i < m_LightPositions.size(); i++) {
////            m_PointLightModels[i].SetPosition(m_LightPositions[i]);
//            m_PointLights[i].position = m_Camera->GetView() * m_LightPositions[i];
//        }
        m_LightsUB->SetData(m_PointLights.data(), 1, 0);

//        for (const auto &lightModel : m_PointLightModels) {
//            lightModel.SetUniform(0, 0, ModelUBO{m_Camera->GetProjectionView() * lightModel.GetModelMatrix()});
//        }

        Entity::UpdateTransformsUB(*m_Camera);
        for (auto *material : m_AssetMaterials)
            material->UpdateUniforms();
    }

    void OnImGuiDraw() override {
        ImGui::Begin("Scene options");
//        ImGui::SliderFloat3("Point Light", &m_SceneUBO.pointLight.position.x, -10, 10);
        ImGui::SliderFloat3("Light direction", &m_SceneUBO.light.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat3("Light ambient", &m_SceneUBO.light.ambient.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Light diffuse", &m_SceneUBO.light.diffuse.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Light specular", &m_SceneUBO.light.specular.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &m_MouseSensitivity, 0.01, 0.3);
        ImGui::SliderFloat("Move speed", &m_MoveSpeed, 0.1f, 5.0f);
        ImGui::End();

//        ImGui::ShowDemoWindow();
    }

    void OnDraw() override {
        Renderer::SubmitCommand(RenderCommand::Clear());
        Renderer::BeginScene(m_Camera);

        for (const auto &entity : m_Entities) {
            for (const auto &meshInstance : entity.MeshRenderers()) {
                Renderer::SubmitMeshRenderer(&meshInstance);
            }
        }


//        Renderer::SubmitModelInstance(&m_TestModelInstance);
//        for (const Model &model : m_ChaletModels) {
//            Renderer::SubmitModel(&model);
//        }
//        for (const Model &model : m_CubeModels) {
//            Renderer::SubmitModel(&model);
//        }
//        for (const Model &light : m_PointLightModels) {
//            Renderer::SubmitModel(&light);
//        }

        Renderer::EndScene();

//        VkCommandBufferBeginInfo beginInfo = {};
//        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
//        beginInfo.pInheritanceInfo = &info;
//
//        vk::CommandBuffer cmdBuffer(m_CmdBuffers[imageIndex]);
//        cmdBuffer.Begin(beginInfo);
//
//        m_RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//        m_RenderPassBeginInfo.renderPass = (VkRenderPass) m_RenderPass->VkHandle();
//        m_RenderPassBeginInfo.renderArea.offset = {0, 0};
//        m_RenderPassBeginInfo.clearValueCount = clearValues.size();
//        m_RenderPassBeginInfo.pClearValues = clearValues.data();
//        m_RenderPassBeginInfo.renderArea.extent = extent;
//        m_RenderPassBeginInfo.framebuffer = m_Renderer.GetFramebuffer().data();
////        vkCmdBeginRenderPass(cmdBuffer.data(), &m_RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//        vkCmdSetViewport(cmdBuffer.data(), 0, 1, &viewport);
//        vkCmdSetScissor(cmdBuffer.data(), 0, 1, &scissor);
//
//        m_GraphicsPipeline->Bind(cmdBuffer.data(), imageIndex);
//
//        m_Mesh->VertexBuffer().Bind(cmdBuffer.data(), 0);
//        m_Mesh->IndexBuffer().Bind(cmdBuffer.data(), 0);
//
//        vkCmdDrawIndexed(cmdBuffer.data(), m_Model->IndexCount(), 1, 0, 0, 0);
//
////        vkCmdEndRenderPass(cmdBuffer.data());
//
//        cmdBuffer.End();
//
//        return cmdBuffer.data();
    }
};


class SandboxApp : public Application {
public:
    SandboxApp() : Application("Sandbox") {
        RendererAPI::SelectAPI(RendererAPI::API::VULKAN);

        PushLayer(std::make_unique<TestLayer>("TestLayer"));
        Renderer::SetImGuiLayer(PushOverlay(ImGuiLayer::Create()));
    }

    ~SandboxApp() override = default;
};


auto Application::CreateApplication() -> std::unique_ptr<Application> { return std::make_unique<SandboxApp>(); }