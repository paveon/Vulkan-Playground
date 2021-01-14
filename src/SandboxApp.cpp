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


class TestLayer : public Layer {
    const char *BACKPACK_MODEL_ASSET_PATH = BASE_DIR "/models/backpack.obj";
    const char *CAR_MODEL_ASSET_PATH = BASE_DIR "/models/car.obj";
    const char *CERBERUS_MODEL_ASSET_PATH = BASE_DIR "/models/Cerberus_LP.FBX";
    const char *MODEL_PATH = BASE_DIR "/models/chalet.obj";
    const char *CHALET_TEXTURE_PATH = BASE_DIR "/textures/chalet.jpg";

    const std::array<const char *, 6> SKYBOX_TEXTURE_PATHS = {
            BASE_DIR "/textures/skybox/right.jpg",
            BASE_DIR "/textures/skybox/left.jpg",
            BASE_DIR "/textures/skybox/top.jpg",
            BASE_DIR "/textures/skybox/bottom.jpg",
            BASE_DIR "/textures/skybox/front.jpg",
            BASE_DIR "/textures/skybox/back.jpg",
    };

    const char *SKYBOX_HDR_TEXTURE = BASE_DIR "/textures/Newport_Loft_Ref.hdr";

    const char *SKYBOX_HDR_TEXTURE_2 = BASE_DIR "/textures/blaubeuren_night_4k.hdr";

    const char *SKYBOX_HDR_TEXTURE_3 = BASE_DIR "/textures/lauter_waterfall_4k.hdr";

    const std::unordered_map<Texture2D::Type, std::pair<const char *, VkFormat>> CONTAINER_TEXTURES = {
            {Texture2D::Type::ALBEDO,   std::make_pair(BASE_DIR "/textures/container_diffuse.png",
                                                       VK_FORMAT_R8G8B8A8_SRGB)},
            {Texture2D::Type::METALLIC, std::make_pair(BASE_DIR "/textures/container_specular.png",
                                                       VK_FORMAT_R8G8B8A8_UNORM)},
    };

    const std::unordered_map<Texture2D::Type, std::pair<const char *, VkFormat>> BRICKWALL_TEXTURES = {
            {Texture2D::Type::ALBEDO, std::make_pair(BASE_DIR "/textures/brickwall_diffuse.jpg",
                                                     VK_FORMAT_R8G8B8A8_SRGB)},
            {Texture2D::Type::NORMAL, std::make_pair(BASE_DIR "/textures/brickwall_normal.jpg",
                                                     VK_FORMAT_R8G8B8A8_UNORM)},
    };

    const std::unordered_map<Texture2D::Type, std::pair<const char *, VkFormat>> RUSTED_IRON_PBR_TEXTURES = {
            {Texture2D::Type::ALBEDO,    std::make_pair(BASE_DIR "/textures/rustediron2_basecolor.png",
                                                        VK_FORMAT_R8G8B8A8_SRGB)},
            {Texture2D::Type::METALLIC,  std::make_pair(BASE_DIR "/textures/rustediron2_metallic.png",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::NORMAL,    std::make_pair(BASE_DIR "/textures/rustediron2_normal.png",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::ROUGHNESS, std::make_pair(BASE_DIR "/textures/rustediron2_roughness.png",
                                                        VK_FORMAT_R8G8B8A8_UNORM)}
    };


    const std::unordered_map<Texture2D::Type, std::pair<const char *, VkFormat>> CERBERUS_PBR_TEXTURES = {
            {Texture2D::Type::ALBEDO,    std::make_pair(BASE_DIR "/textures/cerberus/Cerberus_A.tga",
                                                        VK_FORMAT_R8G8B8A8_SRGB)},
            {Texture2D::Type::METALLIC,  std::make_pair(BASE_DIR "/textures/cerberus/Cerberus_M.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::NORMAL,    std::make_pair(BASE_DIR "/textures/cerberus/Cerberus_N.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::ROUGHNESS, std::make_pair(BASE_DIR "/textures/cerberus/Cerberus_R.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)}
    };

    const std::unordered_map<Texture2D::Type, std::pair<const char *, VkFormat>> CAR_PBR_TEXTURES = {
            {Texture2D::Type::ALBEDO,    std::make_pair(BASE_DIR "/textures/car/car_albedo.tga",
                                                        VK_FORMAT_R8G8B8A8_SRGB)},
            {Texture2D::Type::METALLIC,  std::make_pair(BASE_DIR "/textures/car/car_metallic.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::NORMAL,    std::make_pair(BASE_DIR "/textures/car/car_normal.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)},
            {Texture2D::Type::ROUGHNESS, std::make_pair(BASE_DIR "/textures/car/car_roughness.tga",
                                                        VK_FORMAT_R8G8B8A8_UNORM)}
    };

    const char *phongVertShaderPath = BASE_DIR "/shaders/cube.vert.spv";
    const char *phongFragShaderPath = BASE_DIR "/shaders/cube.frag.spv";

    const char *vertLightShader = BASE_DIR "/shaders/lightCube.vert.spv";
    const char *fragLightShader = BASE_DIR "/shaders/lightCube.frag.spv";

    GfxContextVk &m_Context;
    Device &m_Device;

    std::vector<Texture2D *> m_Textures;
    std::unordered_map<Texture2D::Type, const Texture2D *> m_ContainerTextures;
    std::unordered_map<Texture2D::Type, const Texture2D *> m_BrickwallTextures;
    std::unordered_map<Texture2D::Type, const Texture2D *> m_SphereTextures;
    std::unordered_map<Texture2D::Type, const Texture2D *> m_CerberusTextures;
    std::unordered_map<Texture2D::Type, const Texture2D *> m_CarTextures;
    TextureCubemap *m_SkyboxTexture;
    TextureCubemap *m_SkyboxHdrTexture;
    TextureCubemap *m_SkyboxIrradianceTexture;
    TextureCubemap *m_PrefilteredEnvMap;
    std::unique_ptr<Texture2D> m_BrdfLut;

    std::vector<Material *> m_UsedMaterials;
    std::vector<std::shared_ptr<Material>> m_Materials;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;

    std::vector<std::shared_ptr<ShaderPipeline>> m_Shaders;
    std::vector<std::shared_ptr<ModelAsset>> m_ModelAssets;
    std::vector<Entity> m_Entities;

    std::vector<glm::vec4> m_LightPositions{
            glm::vec4(-10.0f, 10.0f, 10.0f, 1.0f),
            glm::vec4(10.0f, 10.0f, 10.0f, 1.0f),
            glm::vec4(-10.0f, -10.0f, 10.0f, 1.0f),
            glm::vec4(10.0f, -10.0f, 10.0f, 1.0f),
    };
    std::vector<PointLight> m_PointLights;


    SceneUBO m_SceneUBO{
            glm::vec4(0.0f),
            DirectionalLight{
                    glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
                    glm::vec4(0.1f, 0.1f, 0.1f, 0.0f),
                    glm::vec4(0.75f, 0.75f, 0.75f, 0.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)

            },
            SpotLight{
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
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

    std::shared_ptr<Material> m_PbrMaterial;
    std::shared_ptr<Material> m_PbrMaterialStrips;

public:
    explicit TestLayer(const char *name) :
            Layer(name),
            m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
            m_Device(m_Context.GetDevice()),
            m_PointLights(m_LightPositions.size(), {
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.05f, 0.05f, 0.05f, 0.0f),
                    glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
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


        m_SceneUBO.pointLightCount = m_PointLights.size();
        m_SceneUBO.cameraPosition = glm::vec4(m_Camera->GetPosition(), 1.0f);

        size_t ubSize = sizeof(PointLight) * m_PointLights.size();
        m_LightsUB = UniformBuffer::Create("Lights UB", ubSize, 1);
        m_SceneUB = UniformBuffer::Create("Scene UB", sizeof(SceneUBO), 1);

        m_LightsUB->SetData(m_PointLights.data(), 1, 0);
        m_SceneUB->SetData(&m_SceneUBO, 1, 0);


        std::map<ShaderType, const char *> shaderStages{
                {ShaderType::VERTEX_SHADER,   phongVertShaderPath},
                {ShaderType::FRAGMENT_SHADER, phongFragShaderPath}
        };

        DepthState depthState{};
        depthState.testEnable = VK_TRUE;
        depthState.writeEnable = VK_TRUE;
        depthState.boundTestEnable = VK_FALSE;
        depthState.compareOp = VK_COMPARE_OP_LESS;
        depthState.min = 0.0f;
        depthState.max = 1.0f;

        MultisampleState msState{};
        msState.sampleCount = m_Device.maxSamples();
        msState.sampleShadingEnable = VK_TRUE;
        msState.minSampleShading = 0.2f;

        m_Shaders.emplace_back(ShaderPipeline::Create("PBR Shader",
                                                      shaderStages,
                                                      {{0, 0},
                                                       {4, 0}},
                                                      Renderer::GetRenderPass(),
                                                      0,
                                                      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                                      {VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE},
                                                      depthState,
                                                      msState));
        auto pbrShader = m_Shaders.back();

        m_Shaders.emplace_back(ShaderPipeline::Create("PBR Shader Triangle Strips",
                                                      shaderStages,
                                                      {{0, 0},
                                                       {4, 0}},
                                                      Renderer::GetRenderPass(),
                                                      0,
                                                      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                                                      {VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE},
                                                      depthState,
                                                      msState));
        auto pbrShaderStrips = m_Shaders.back();

        m_SkyboxHdrTexture = TextureCubemap::CreateFromHDR(SKYBOX_HDR_TEXTURE, 512);
        m_SkyboxIrradianceTexture = m_SkyboxHdrTexture->CreateIrradianceCubemap(256);
        m_PrefilteredEnvMap = m_SkyboxHdrTexture->CreatePrefilteredCubemap(512, 5);
        m_BrdfLut = Texture2D::GenerateBrdfLut(512);
//        m_SkyboxTexture = TextureCubemap::Create(SKYBOX_TEXTURE_PATHS);
        Renderer::SetSkybox(m_SkyboxHdrTexture);

        for (const auto&[type, tex] : CERBERUS_PBR_TEXTURES) {
            m_CerberusTextures.emplace(type, Texture2D::Create(tex.first, tex.second, false));
        }
        for (const auto&[type, tex] : CAR_PBR_TEXTURES) {
            m_CarTextures.emplace(type, Texture2D::Create(tex.first, tex.second, false));
        }
        for (const auto&[type, tex] : BRICKWALL_TEXTURES) {
            m_BrickwallTextures.emplace(type, Texture2D::Create(tex.first, tex.second, true));
        }
        for (const auto&[type, tex] : RUSTED_IRON_PBR_TEXTURES) {
            m_SphereTextures.emplace(type, Texture2D::Create(tex.first, tex.second, true));
        }
        m_SphereTextures[Texture2D::Type::BRDF_LUT] = m_BrdfLut.get();
        m_CerberusTextures[Texture2D::Type::BRDF_LUT] = m_BrdfLut.get();
        m_CarTextures[Texture2D::Type::BRDF_LUT] = m_BrdfLut.get();
        m_BrickwallTextures[Texture2D::Type::BRDF_LUT] = m_BrdfLut.get();

        BindingKey materialKey(4, 0);

        m_Materials.emplace_back(std::make_shared<Material>("PBR Material", pbrShader));
        m_PbrMaterial = m_Materials.back();
        m_UsedMaterials.push_back(m_Materials.back().get());

        m_Materials.emplace_back(std::make_shared<Material>("PBR Material (Triangle Strips)", pbrShaderStrips));
        m_PbrMaterialStrips = m_Materials.back();
        m_UsedMaterials.push_back(m_Materials.back().get());

//        m_BrickwallTextures[Texture2D::Type::ALBEDO] = m_BrdfLut.get();
        auto cerberusTexIndices = m_PbrMaterial->BindTextures(m_CerberusTextures, BindingKey(1, 0));
        auto carTexIndices = m_PbrMaterial->BindTextures(m_CarTextures, BindingKey(1, 0));
        auto brickwallTexIndices = m_PbrMaterial->BindTextures(m_BrickwallTextures, BindingKey(1, 0));
        auto cubemapTexIndices = m_PbrMaterial->BindCubemaps(
                {
                        {TextureCubemap::Type::ENVIRONMENT,     m_SkyboxHdrTexture},
                        {TextureCubemap::Type::IRRADIANCE,      m_SkyboxIrradianceTexture},
                        {TextureCubemap::Type::PREFILTERED_ENV, m_PrefilteredEnvMap}
                },
                BindingKey(1, 1)
        );

        m_PbrMaterial->SetUniform(materialKey, "albedo", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
        m_PbrMaterial->SetUniform(materialKey, "ao", 1.0f);
        m_PbrMaterial->SetUniform(materialKey, "metallic", 0.8f);
        m_PbrMaterial->SetUniform(materialKey, "roughness", 0.25f);

        m_PbrMaterial->SetUniform(materialKey, "environmentMapTexIdx",
                                  cubemapTexIndices[TextureCubemap::Type::ENVIRONMENT]);
        m_PbrMaterial->SetUniform(materialKey, "irradianceMapTexIdx",
                                  cubemapTexIndices[TextureCubemap::Type::IRRADIANCE]);
        m_PbrMaterial->SetUniform(materialKey, "prefilterMapTexIdx",
                                  cubemapTexIndices[TextureCubemap::Type::PREFILTERED_ENV]);

        m_PbrMaterial->SetUniform(materialKey, "albedoMapTexIdx", cerberusTexIndices[Texture2D::Type::ALBEDO]);
        m_PbrMaterial->SetUniform(materialKey, "normalMapTexIdx", cerberusTexIndices[Texture2D::Type::NORMAL]);
        m_PbrMaterial->SetUniform(materialKey, "metallicMapTexIdx", cerberusTexIndices[Texture2D::Type::METALLIC]);
        m_PbrMaterial->SetUniform(materialKey, "roughnessMapTexIdx", cerberusTexIndices[Texture2D::Type::ROUGHNESS]);
        m_PbrMaterial->SetUniform(materialKey, "brdfLutIdx", cerberusTexIndices[Texture2D::Type::BRDF_LUT]);
        m_PbrMaterial->SetUniform(materialKey, "aoMapTexIdx", -1);


        auto sphereTexIndices = m_PbrMaterialStrips->BindTextures(m_SphereTextures, BindingKey(1, 0));
        cubemapTexIndices = m_PbrMaterialStrips->BindCubemaps(
                {
                        {TextureCubemap::Type::ENVIRONMENT,     m_SkyboxHdrTexture},
                        {TextureCubemap::Type::IRRADIANCE,      m_SkyboxIrradianceTexture},
                        {TextureCubemap::Type::PREFILTERED_ENV, m_PrefilteredEnvMap}
                },
                BindingKey(1, 1)
        );

        m_PbrMaterialStrips->SetUniform(materialKey, "diffuseTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "specularTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "shininess", 32.0f);
        m_PbrMaterialStrips->SetUniform(materialKey, "albedo", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
        m_PbrMaterialStrips->SetUniform(materialKey, "ao", 1.0f);
        m_PbrMaterialStrips->SetUniform(materialKey, "metallic", 0.8f);
        m_PbrMaterialStrips->SetUniform(materialKey, "roughness", 0.25f);

        // PBR textures
        m_PbrMaterialStrips->SetUniform(materialKey, "environmentMapTexIdx",
                                        cubemapTexIndices[TextureCubemap::Type::ENVIRONMENT]);
        m_PbrMaterialStrips->SetUniform(materialKey, "irradianceMapTexIdx",
                                        cubemapTexIndices[TextureCubemap::Type::IRRADIANCE]);
        m_PbrMaterialStrips->SetUniform(materialKey, "prefilterMapTexIdx",
                                        cubemapTexIndices[TextureCubemap::Type::PREFILTERED_ENV]);

//        m_PbrMaterialStrips->SetUniform(materialKey, "albedoMapTexIdx", sphereTexIndices[Texture2D::Type::ALBEDO]);
//        m_PbrMaterialStrips->SetUniform(materialKey, "normalMapTexIdx", sphereTexIndices[Texture2D::Type::NORMAL]);
//        m_PbrMaterialStrips->SetUniform(materialKey, "metallicMapTexIdx", sphereTexIndices[Texture2D::Type::METALLIC]);
//        m_PbrMaterialStrips->SetUniform(materialKey, "roughnessMapTexIdx",
//                                        sphereTexIndices[Texture2D::Type::ROUGHNESS]);
        m_PbrMaterialStrips->SetUniform(materialKey, "brdfLutIdx", sphereTexIndices[Texture2D::Type::BRDF_LUT]);
        m_PbrMaterialStrips->SetUniform(materialKey, "albedoMapTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "normalMapTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "metallicMapTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "roughnessMapTexIdx", -1);
        m_PbrMaterialStrips->SetUniform(materialKey, "aoMapTexIdx", -1);


//        m_Materials[0]->AllocateResources(10);
//        m_Materials[1]->AllocateResources(100);
//        m_Materials[2]->AllocateResources(10);

//        auto indicesMat0 = m_Materials[0]->BindTextures({m_Textures[0].get()}, 3, 0);
//        auto indicesMat1 = m_Materials[1]->BindTextures({m_Textures[1].get(), m_Textures[2].get(), m_Textures[3].get()},
//                                                        3, 0);


        m_Entities.emplace_back("Cerberus");
        m_ModelAssets.emplace_back(ModelAsset::LoadModel(CERBERUS_MODEL_ASSET_PATH));
        auto cerberusAsset = m_ModelAssets.back();
        cerberusAsset->StageMeshes();
        auto &cerberusEntity = m_Entities.back();
        cerberusEntity.SetScale(glm::vec3(0.03f));
        cerberusEntity.SetPosition(glm::vec3(3.0f));
        cerberusEntity.SetRotationX(-HALF_PI_F);
        auto &meshInstance = cerberusEntity.AttachMesh(cerberusAsset->Meshes().back());
        meshInstance.SetMaterialInstance(m_PbrMaterial->CreateInstance());
        auto &materialInstance = meshInstance.GetMaterialInstance();


        m_Entities.emplace_back("Car");
        m_ModelAssets.emplace_back(ModelAsset::LoadModel(CAR_MODEL_ASSET_PATH));
        auto carAsset = m_ModelAssets.back();
        carAsset->StageMeshes();
        auto &carEntity = m_Entities.back();
        carEntity.SetScale(glm::vec3(0.7f));
        carEntity.SetPosition(glm::vec3(-1.0f, 0.0f, 3.0f));
        carEntity.SetRotationY(HALF_PI_F);
        auto &carMeshInstance = carEntity.AttachMesh(carAsset->Meshes().back());
        carMeshInstance.SetMaterialInstance(m_PbrMaterial->CreateInstance());
        auto &carMaterialInstance = carMeshInstance.GetMaterialInstance();
        carMaterialInstance.SetUniform(materialKey, "metallicMapTexIdx", carTexIndices[Texture2D::Type::METALLIC]);
        carMaterialInstance.SetUniform(materialKey, "roughnessMapTexIdx", carTexIndices[Texture2D::Type::ROUGHNESS]);
//        carMaterialInstance.SetUniform(materialKey, "metallicMapTexIdx", -1);
//        carMaterialInstance.SetUniform(materialKey, "roughnessMapTexIdx", -1);
        carMaterialInstance.SetUniform(materialKey, "albedoMapTexIdx", carTexIndices[Texture2D::Type::ALBEDO]);
        carMaterialInstance.SetUniform(materialKey, "normalMapTexIdx", carTexIndices[Texture2D::Type::NORMAL]);
        carMaterialInstance.SetUniform(materialKey, "metallic", 1.0f);
        carMaterialInstance.SetUniform(materialKey, "roughness", 0.0f);


//        m_ModelAssets.emplace_back(ModelAsset::LoadModel(CERBERUS_MODEL_ASSET_PATH));
//        auto backpackAsset = m_ModelAssets.back();
//        auto &backpackMaterials = backpackAsset->GetMaterials();
//        backpackAsset->StageMeshes();
//        for (size_t i = 0; i < backpackMaterials.size(); i++) {
//            m_UsedMaterials.push_back(&backpackMaterials[i]);
//            backpackMaterials[i].SetShaderPipeline(pbrShader);
//
//            m_UsedMaterials.back()->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
//            m_UsedMaterials.back()->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
//
//            cubemapTexIndices = backpackMaterials[i].BindCubemaps(
//                    {
//                            {TextureCubemap::Type::ENVIRONMENT, m_SkyboxTexture},
//                            {TextureCubemap::Type::IRRADIANCE,  m_SkyboxIrradianceTexture}
//                    },
//                    BindingKey(1, 1)
//            );
//            backpackMaterials[i].SetUniform(materialKey, "shininess", 32.0f);
//
//            backpackMaterials[i].SetUniform(materialKey, "environmentMapTexIdx",
//                                            cubemapTexIndices[TextureCubemap::Type::ENVIRONMENT]);
//
//            backpackMaterials[i].SetUniform(materialKey, "irradianceMapTexIdx",
//                                            cubemapTexIndices[TextureCubemap::Type::IRRADIANCE]);
//
//            backpackMaterials[i].SetUniform(materialKey, "normalMapTexIdx", -1);
//            auto textures = backpackAsset->TextureMap(i);
//            auto texIndices = backpackMaterials[i].BindTextures(textures, BindingKey(1, 0));
//            for (const auto&[type, texIdx] : texIndices) {
//                switch (type) {
//                    case Texture2D::Type::SPECULAR:
//                        backpackMaterials[i].SetUniform(materialKey, "specularTexIdx", texIdx);
//                        break;
//                    case Texture2D::Type::DIFFUSE:
//                        backpackMaterials[i].SetUniform(materialKey, "diffuseTexIdx", texIdx);
//                        break;
//                    case Texture2D::Type::NORMAL:
//                        backpackMaterials[i].SetUniform(materialKey, "normalMapTexIdx", texIdx);
//                        break;
//                }
//            }
//        }

        Entity::AllocateTransformsUB(256);

//        m_Entities.emplace_back("Backpack", *m_ModelAssets.back());
//        m_Entities.back().SetScale(glm::vec3(0.5f));

        m_ModelAssets.emplace_back(ModelAsset::CreateCubeAsset());
        auto cubeAsset = m_ModelAssets.back();
        cubeAsset->StageMeshes();

        m_ModelAssets.emplace_back(ModelAsset::CreateQuadAsset());
        auto quadAsset = m_ModelAssets.back();
        quadAsset->StageMeshes();

        m_ModelAssets.emplace_back(ModelAsset::CreateSphereAsset());
        auto sphereAsset = m_ModelAssets.back();
        sphereAsset->StageMeshes();


//        m_Entities.emplace_back("Cube");
//        auto &cubeObject = m_Entities.back();
//        auto &meshInstance = cubeObject.AttachMesh(cubeAsset->Meshes().back());
//        meshInstance.SetMaterialInstance(m_Materials[0]->CreateInstance());
//        cubeObject.SetScale(glm::vec3(0.5f));
//        cubeObject.SetPosition(glm::vec3(1.0f));

        size_t nrRows = 7;
        size_t nrColumns = 7;
        float spacing = 1.5f;

        for (size_t row = 0; row < nrRows; ++row) {
            for (size_t col = 0; col < nrColumns; ++col) {
                std::ostringstream sphereName;
                sphereName << "Sphere (" << row << ";" << col << ")";
                m_Entities.emplace_back(sphereName.str());
                auto &sphereObject = m_Entities.back();
                auto &meshInstance = sphereObject.AttachMesh(sphereAsset->Meshes().back());
                meshInstance.SetMaterialInstance(m_PbrMaterialStrips->CreateInstance());
                auto &materialInstance = meshInstance.GetMaterialInstance();

                float roughness = glm::clamp((float) col / (float) nrColumns, 0.05f, 1.0f);
                materialInstance.SetUniform(materialKey, "metallic", (float) row / (float) nrRows);
                materialInstance.SetUniform(materialKey, "roughness", roughness);

                sphereObject.SetScale(glm::vec3(0.5f));
                sphereObject.SetPosition(glm::vec3(
                        (col - (nrColumns / 2.0f)) * spacing,
                        (row - (nrRows / 2.0f)) * spacing,
                        0.0f)
                );
            }
        }


//        m_Entities.emplace_back("Quad");
//        auto &quadObject = m_Entities.back();
//        auto &quadMeshInstance = quadObject.AttachMesh(quadAsset->Meshes().back());
//        quadMeshInstance.SetMaterialInstance(m_PbrMaterial->CreateInstance());
//        quadObject.SetScale(glm::vec3(10.0f, 1.0f, 10.0f));
//        quadObject.SetPosition(glm::vec3(0.0f, -1.0f, 0.0f));
//
//        auto &quadMaterialInstance = quadMeshInstance.GetMaterialInstance();
//        quadMaterialInstance.SetUniform(materialKey, "metallicMapTexIdx", -1);
//        quadMaterialInstance.SetUniform(materialKey, "roughnessMapTexIdx", -1);
//        quadMaterialInstance.SetUniform(materialKey, "albedoMapTexIdx", brickwallTexIndices[Texture2D::Type::ALBEDO]);
//        quadMaterialInstance.SetUniform(materialKey, "normalMapTexIdx", brickwallTexIndices[Texture2D::Type::NORMAL]);
//        quadMaterialInstance.SetUniform(materialKey, "brdfLutIdx", brickwallTexIndices[Texture2D::Type::BRDF_LUT]);
//        quadMaterialInstance.SetUniform(materialKey, "ao", 1.0f);
//        quadMaterialInstance.SetUniform(materialKey, "metallic", 0.15f);
//        quadMaterialInstance.SetUniform(materialKey, "roughness", 1.0f);
//        quadMaterialInstance.SetUniform(materialKey, "environmentMapTexIdx",
//                                        cubemapTexIndices[TextureCubemap::Type::ENVIRONMENT]);
//        quadMaterialInstance.SetUniform(materialKey, "irradianceMapTexIdx",
//                                        cubemapTexIndices[TextureCubemap::Type::IRRADIANCE]);
//        quadMaterialInstance.SetUniform(materialKey, "prefilterMapTexIdx",
//                                        cubemapTexIndices[TextureCubemap::Type::PREFILTERED_ENV]);
//
//        quadMaterialInstance.SetUniform(materialKey, "aoMapTexIdx", -1);


//        m_SkyboxMesh = cubeAsset->Meshes().back().CreateInstance(0);
//        m_SkyboxMesh.SetMaterialInstance(m_SkyboxMaterial->CreateInstance());


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

        pbrShader->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
        pbrShaderStrips->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
//        m_Shaders[1]->BindUniformBuffer(Entity::TransformsUB(), BindingKey(0, 0));
        m_PbrMaterial->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
        m_PbrMaterial->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
        m_PbrMaterialStrips->BindUniformBuffer(m_SceneUB.get(), BindingKey(2, 0));
        m_PbrMaterialStrips->BindUniformBuffer(m_LightsUB.get(), BindingKey(3, 0));
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
            m_Camera->MoveY(m_MoveSpeed * ts);
        } else if (Input::KeyPressed(IO_KEY_Q)) {
            m_Camera->MoveY(-m_MoveSpeed * ts);
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
        sceneUBO.light.direction.w = 0.0f;
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

        for (size_t i = 0; i < m_LightPositions.size(); i++) {
//            m_PointLightModels[i].SetPosition(m_LightPositions[i]);
//            m_PointLights[i].position = m_Camera->GetView() * m_LightPositions[i];
            m_PointLights[i].position = m_LightPositions[i];
        }
        m_LightsUB->SetData(m_PointLights.data(), 1, 0);

//        for (const auto &lightModel : m_PointLightModels) {
//            lightModel.SetUniform(0, 0, ModelUBO{m_Camera->GetProjectionView() * lightModel.GetModelMatrix()});
//        }

        Entity::UpdateTransformsUB(*m_Camera);
        for (auto *material : m_UsedMaterials)
            material->UpdateUniforms();
    }

    void OnImGuiDraw() override {
        static Entity *selectedEntity = nullptr;
        static bool useNormalMap = true;
        static bool enableSkybox = true;
        static float exposure = 1.0f;
        static float skyboxLOD = 0.0f;

//        ImGui::Begin("Docking space");
//        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
//        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
//        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
//        ImGui::End();


        ImGui::Begin("Scene options");
//        ImGui::SliderFloat3("Point Light", &m_SceneUBO.pointLight.position.x, -10, 10);
//        ImGui::SliderFloat3("Light direction", &m_SceneUBO.light.direction.x, -1.0f, 1.0f);
//        ImGui::SliderFloat3("Light ambient", &m_SceneUBO.light.ambient.x, 0.0f, 1.0f);
//        ImGui::SliderFloat3("Light diffuse", &m_SceneUBO.light.diffuse.x, 0.0f, 1.0f);
//        ImGui::SliderFloat3("Light specular", &m_SceneUBO.light.specular.x, 0.0f, 1.0f);
        ImGui::DragFloat("Mouse Sensitivity", &m_MouseSensitivity, 0.05, 0.01, 1.0f);
        ImGui::DragFloat("Move speed", &m_MoveSpeed, 0.1f, 0.0f, 100.0f);
        if (ImGui::DragFloat("Exposure", &exposure, 0.1f, 0.1f, 10.0f)) {
            Renderer::SetExposure(exposure);
        }

        if (ImGui::Checkbox("Enable Skybox", &enableSkybox)) {
            if (enableSkybox) Renderer::EnableSkybox();
            else Renderer::DisableSkybox();
        }
        if (ImGui::DragFloat("Skybox LOD", &skyboxLOD, 0.1f, 0.0f, 10.0f)) {
            Renderer::SetSkyboxLOD(skyboxLOD);
        }

        if (ImGui::TreeNode("Entities")) {
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                   ImGuiTreeNodeFlags_SpanAvailWidth;

            Entity *clickedEntity = nullptr;
            for (auto &entity : m_Entities) {
                ImGuiTreeNodeFlags node_flags =
                        base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
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

        ImGui::Begin("Properties");
        if (selectedEntity) {
            if (ImGui::Checkbox("Use Normal map", &useNormalMap)) {
                auto &instance = selectedEntity->MeshRenderers()[0].GetMaterialInstance();
                if (useNormalMap) {
                    // TODO: create some basic UI for texture enabling/disabling
                    instance.SetUniform({4, 0}, "normalMapTexIdx", -1);
                } else {
                    instance.SetUniform({4, 0}, "normalMapTexIdx", -1);
                }
            }

            if (ImGui::CollapsingHeader("Transform")) {
                glm::vec3 pos = selectedEntity->GetPosition();
                glm::vec3 scale = selectedEntity->GetScale();
                glm::vec3 rotation = selectedEntity->GetRotation();

                if (ImGui::DragFloat3("Translation", &pos.x, 0.1f, 1.0f, 0.0f)) {
                    selectedEntity->SetPosition(pos);
                }
                if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f, -PI_F, PI_F)) {
                    selectedEntity->SetRotation(rotation);
                }
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 1.0f, 0.0f)) {
                    selectedEntity->SetScale(scale);
                }
            }
        }
        ImGui::End();

        ImGui::ShowDemoWindow();
    }

    void OnDraw() override {
        Renderer::SubmitCommand(RenderCommand::Clear());
        Renderer::BeginScene(m_Camera);

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