#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>
#include <Platform/Vulkan/GraphicsContextVk.h>
#include <glm/matrix.hpp>


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

    const std::array<const char*, 6> SKYBOX_TEXTURE_PATHS = {
            BASE_DIR "/textures/skybox/right.jpg",
            BASE_DIR "/textures/skybox/left.jpg",
            BASE_DIR "/textures/skybox/top.jpg",
            BASE_DIR "/textures/skybox/bottom.jpg",
            BASE_DIR "/textures/skybox/front.jpg",
            BASE_DIR "/textures/skybox/back.jpg"
    };
//    const char *CONTAINER_EMISSION_TEX_PATH = BASE_DIR "/textures/matrix_emission_map.jpg";

//    const char *vertShaderPath = BASE_DIR "/shaders/triangle.vert.spv";
//    const char *fragShaderPath = BASE_DIR "/shaders/triangle.frag.spv";

    const char *phongVertShaderPath = BASE_DIR "/shaders/cube.vert.spv";
    const char *phongFragShaderPath = BASE_DIR "/shaders/cube.frag.spv";

    const char *skyboxVertShaderPath = BASE_DIR "/shaders/skybox.vert.spv";
    const char *skyboxFragShaderPath = BASE_DIR "/shaders/skybox.frag.spv";

    const char *vertLightShader = BASE_DIR "/shaders/lightCube.vert.spv";
    const char *fragLightShader = BASE_DIR "/shaders/lightCube.frag.spv";

    GfxContextVk &m_Context;
    Device &m_Device;

    std::vector<Texture2D*> m_Textures;
    TextureCubemap* m_SkyboxTexture;

    std::vector<Material *> m_UsedMaterials;
    std::vector<std::shared_ptr<Material>> m_Materials;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;

    std::vector<std::shared_ptr<ShaderPipeline>> m_Shaders;
    std::vector<std::unique_ptr<ModelAsset>> m_ModelAssets;
    std::vector<Entity> m_Entities;
//    std::unique_ptr<MeshRenderer> m_SkyboxMesh;
    MeshRenderer m_SkyboxMesh;


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
                    glm::vec4(0.4f, 0.4f, 0.4f, 0.0f),
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
                0.1f, 20.0f, glm::radians(45.0f));
    }

    void OnAttach(const LayerStack *stack) override {
        Layer::OnAttach(stack);

        auto[width, height] = Application::GetGraphicsContext().FramebufferSize();
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

        DepthState depthState{};
        depthState.testEnable = VK_TRUE;
        depthState.writeEnable = VK_TRUE;
        depthState.boundTestEnable = VK_FALSE;
        depthState.compareOp = VK_COMPARE_OP_LESS;
        depthState.min = 0.0f;
        depthState.max = 1.0f;
        m_Shaders.emplace_back(ShaderPipeline::Create("PhongShader",
                                                      shaderStages,
                                                      {{0, 0},
                                                       {4, 0}},
                                                      Renderer::GetRenderPass(),
                                                      0,
                                                      {VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE},
                                                      depthState));

        depthState.testEnable = VK_FALSE;
        depthState.writeEnable = VK_FALSE;
        depthState.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        shaderStages[0] = {skyboxVertShaderPath, ShaderType::VERTEX_SHADER};
        shaderStages[1] = {skyboxFragShaderPath, ShaderType::FRAGMENT_SHADER};
        m_Shaders.emplace_back(ShaderPipeline::Create("SkyboxShader",
                                                      shaderStages,
                                                      {},
                                                      Renderer::GetRenderPass(),
                                                      0,
                                                      {VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE},
                                                      depthState));

        m_SkyboxTexture = TextureCubemap::Create(SKYBOX_TEXTURE_PATHS);

        m_Textures.emplace_back(Texture2D::Create(CHALET_TEXTURE_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_DIFFUSE_TEX_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_SPECULAR_TEX_PATH));
        m_Textures.emplace_back(Texture2D::Create(CONTAINER_EMISSION_TEX_PATH));

//        m_Materials.emplace_back(std::make_shared<Material>("cubeMaterial", m_Shaders[0]));
//        m_Materials.emplace_back(std::make_shared<Material>("lightMaterial", m_Shaders[1]));

        m_Materials.emplace_back(std::make_shared<Material>("Cube Material", m_Shaders[0]));
        auto &cubeMaterial = m_Materials.back();
        auto texIndices = cubeMaterial->BindTextures(
                {{Texture2D::Type::DIFFUSE,  m_Textures[1]},
                 {Texture2D::Type::SPECULAR, m_Textures[2]}},
                BindingKey(1, 0)
        );
        m_Materials.back()->SetUniform(BindingKey{4, 0}, "diffuseTexIdx", texIndices[0]);
        m_Materials.back()->SetUniform(BindingKey{4, 0}, "specularTexIdx", texIndices[1]);
        m_Materials.back()->SetUniform(BindingKey{4, 0}, "shininess", 32.0f);
        m_UsedMaterials.push_back(m_Materials.back().get());


        m_Materials.emplace_back(std::make_shared<Material>("Skybox Material", m_Shaders[1]));
        auto &skyboxMaterial = m_Materials.back();
        uint32_t skyboxTexIdx = skyboxMaterial->BindCubemap(m_SkyboxTexture, BindingKey(1, 0));
        skyboxMaterial->SetUniform(BindingKey{1, 1}, "skyboxTexIdx", skyboxTexIdx);
        m_UsedMaterials.push_back(m_Materials.back().get());

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
        for (size_t i = 0; i < backpackMaterials.size(); i++) {
            m_UsedMaterials.push_back(&backpackMaterials[i]);
            backpackMaterials[i].SetShaderPipeline(m_Shaders[0]);
            backpackMaterials[i].SetUniform(BindingKey(4, 0), "shininess", 32.0f);
            auto textures = backpackAsset->TextureVector(i);
            texIndices = backpackMaterials[i].BindTextures(textures, BindingKey(1, 0));
            for (size_t texIdx = 0; texIdx < textures.size(); texIdx++) {
                switch (textures[texIdx].first) {
                    case Texture2D::Type::SPECULAR:
                        backpackMaterials[i].SetUniform(BindingKey(4, 0), "specularTexIdx", texIndices[texIdx]);
                        break;
                    case Texture2D::Type::DIFFUSE:
                        backpackMaterials[i].SetUniform(BindingKey(4, 0), "diffuseTexIdx", texIndices[texIdx]);
                        break;
                }
            }
        }

        Entity::AllocateTransformsUB(256);

        m_Entities.emplace_back("Backpack", m_ModelAssets.back());
        m_Entities.back().SetScale(glm::vec3(0.5f));
        m_Entities.back().SetRotationX(HALF_PI_F);
        m_Entities.back().SetRotationY(PI_F);

        m_ModelAssets.emplace_back(ModelAsset::CreateCubeAsset());
        auto &cubeAsset = m_ModelAssets.back();
        cubeAsset->StageMeshes();


        m_Entities.emplace_back("Cube");
        auto &cubeObject = m_Entities.back();
        for (auto &mesh : cubeAsset->Meshes()) {
            auto &meshInstance = cubeObject.AttachMesh(mesh);
            meshInstance.SetMaterialInstance(m_Materials[0]->CreateInstance());
        }
        cubeObject.SetScale(glm::vec3(0.5f));
        cubeObject.SetPosition(glm::vec3(1.0f));

//        m_Entities.emplace_back("Skybox Cube");
//        m_Entities.emplace(m_Entities.begin(), "Skybox Cube");
//        auto &skyboxObject = m_Entities.front();
        for (auto &mesh : cubeAsset->Meshes()) {
            m_SkyboxMesh = mesh.CreateInstance(0);
            m_SkyboxMesh.SetMaterialInstance(m_UsedMaterials[1]->CreateInstance());
        }

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

        m_Shaders[0]->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
//        m_Shaders[1]->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
        m_UsedMaterials[0]->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
        m_UsedMaterials[0]->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
        m_UsedMaterials[2]->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
        m_UsedMaterials[2]->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
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

//        m_Entities.back().SetRotationX(HALF_PI_F);
//        m_Entities.back().SetRotationY(PI_F);
        auto rotX = glm::rotate(glm::mat4(1.0f), HALF_PI_F, glm::vec3(1.0f, 0.0f, 0.0f));
        auto rotXY = glm::rotate(rotX, PI_F, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 skyboxView = glm::mat3(m_Camera->GetView());
        TransformUBO skyboxTransform{};
        skyboxTransform.mvp = m_Camera->GetProjection() * skyboxView * rotXY;
        m_UsedMaterials[1]->SetUniform(BindingKey(0,0), "mvp", skyboxTransform.mvp);

        Entity::UpdateTransformsUB(*m_Camera);
        for (auto *material : m_UsedMaterials)
            material->UpdateUniforms();
    }

    void OnImGuiDraw() override {
        static const Entity *selectedEntity = nullptr;

        ImGui::Begin("Docking space");
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        ImGui::End();


        ImGui::Begin("Scene options");

//        ImGui::SliderFloat3("Point Light", &m_SceneUBO.pointLight.position.x, -10, 10);
        ImGui::SliderFloat3("Light direction", &m_SceneUBO.light.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat3("Light ambient", &m_SceneUBO.light.ambient.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Light diffuse", &m_SceneUBO.light.diffuse.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Light specular", &m_SceneUBO.light.specular.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &m_MouseSensitivity, 0.01, 0.3);
        ImGui::SliderFloat("Move speed", &m_MoveSpeed, 0.1f, 5.0f);

        if (ImGui::TreeNode("Entities")) {
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                   ImGuiTreeNodeFlags_SpanAvailWidth;

            const Entity *clickedEntity = nullptr;
            for (const auto &entity : m_Entities) {
                ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (&entity == selectedEntity) {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx(&entity, node_flags, "%s", entity.m_Name.c_str());
                if (ImGui::IsItemClicked()) {
                    clickedEntity = &entity;
                }
            }

            if (clickedEntity) {
                if (ImGui::GetIO().KeyCtrl) {
                    selectedEntity = selectedEntity ? nullptr : clickedEntity;
                } else {
                    selectedEntity = clickedEntity;
                }
            }
            ImGui::TreePop();
        }

        ImGui::End();

//        ImGui::ShowDemoWindow();
    }

    void OnDraw() override {
        Renderer::SubmitCommand(RenderCommand::Clear());
        Renderer::BeginScene(m_Camera);

        Renderer::SubmitSkybox(&m_SkyboxMesh);

        for (const auto &entity : m_Entities) {
            for (const auto &meshInstance : entity.MeshRenderers()) {
                Renderer::SubmitMeshRenderer(&meshInstance);
            }
        }

        Renderer::EndScene();
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