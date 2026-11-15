#include "pch.h"
#include "PBRScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/MeshFactory.h"
#include "../JamEngine/ModelImporter.h"

#define SCENE_PATH(_filepath) "Resources/PBRScene/" _filepath

namespace
{

constexpr Int32 kLinearClampSlot = 0;
constexpr Int32 kLinearWrapSlot  = 1;

constexpr Int32 kHdrSceneSlot            = 0;
constexpr Int32 kIblIrradianceSlot       = 0;
constexpr Int32 kIblSpecularSlot         = 1;
constexpr Int32 kIblEnvironmentSlot      = 2;
constexpr Int32 kBrdfLUTSlot             = 2;
constexpr Int32 kAlbedoSlot              = 3;
constexpr Int32 kAoRoughnessMetallicSlot = 4;
constexpr Int32 kEmissiveSlot            = 5;
constexpr Int32 kNormalSlot              = 6;

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec2  uvScale;
    float padding[2];
};

struct CBMaterial
{
    static constexpr Int32 kSlot = 3;

    Vec3   albedo;
    float  roughness;
    float  metallic;
    UInt32 flags;
    float  padding[2];
};

constexpr UInt32 kMaterialAlbedoMap   = 1 << 0;
constexpr UInt32 kMaterialNormalMap   = 1 << 1;
constexpr UInt32 kMaterialArmMap      = 1 << 2;
constexpr UInt32 kMaterialEmissiveMap = 1 << 3;

constexpr const char* kTextureSetNames[] = {
    "brown_planks_03",
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding1;
    Mtx4  viewProj;
};

struct CBLights
{
    static constexpr Int32 kSlot = 2;

    struct Light
    {
        Vec3  lightPos;
        float padding;

        Vec3  lightDir;
        float fallOffStart;

        Vec3  lightColor;
        float fallOffEnd;

        float innerConeCos;
        float outerConeCos;
        Int32 bEnabled;
        Int32 type;   // 0: directional, 1: point, 2: spot
    };

    Light lights[4];
};

struct CBCubeMap
{
    constexpr static Int32 kSlot = 2;

    UInt32 type;   // 0: none, 1: irradiance, 2: specular, 3: environment
    Vec3   backgroundColor;
};

}   // namespace

PBRScene::PBRScene()
{
    Graphics& gfx = GetGraphics();

    LoadEnvironment_(0);
    m_pBRDFLUT = LoadTexture(SCENE_PATH("IBLBrdf.dds"));

    // cube for skybox
    {
        Vec3 vertices[8] = {
            { -1.f,  1.f, -1.f },
            {  1.f,  1.f, -1.f },
            {  1.f, -1.f, -1.f },
            { -1.f, -1.f, -1.f },
            { -1.f,  1.f,  1.f },
            {  1.f,  1.f,  1.f },
            {  1.f, -1.f,  1.f },
            { -1.f, -1.f,  1.f }
        };
        UInt16 indices[] = { 0, 1, 2, 0, 2, 3, 1, 5, 6, 1, 6, 2, 5, 4, 7, 5, 7, 6, 4, 0, 3, 4, 3, 7, 4, 5, 1, 4, 1, 0, 3, 2, 6, 3, 6, 7 };

        VertexLayout layout = {};
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);

        m_cubeVbh = gfx.CreateVertexBuffer(AsMemoryView(vertices), layout);
        m_cubeIbh = gfx.CreateIndexBuffer(AsMemoryView(indices));
    }

    InitObjects_();
    InitRenderGraph_();

    m_camera.camera.SetAspectRatio(16.f / 9.f);
    m_camera.camera.SetFarZ(200.f);
    m_camera.transform.SetTranslation({ 0.f, 11.5f, -14.f });
    m_camera.euler.pitch = ToRad(29.f);
    m_camera.transform.SetRotation(m_camera.euler);

    m_lights[0].bEnabled     = true;
    m_lights[0].type         = eLight::Point;
    m_lights[0].color        = Vec3 { 4.f, 2.8f, 1.6f };
    m_lights[0].fallOffStart = 4.f;
    m_lights[0].fallOffEnd   = 22.f;

    m_lights[1].bEnabled     = true;
    m_lights[1].type         = eLight::Point;
    m_lights[1].color        = Vec3 { 1.4f, 2.4f, 4.5f };
    m_lights[1].fallOffStart = 4.f;
    m_lights[1].fallOffEnd   = 22.f;
}

PBRScene::~PBRScene() noexcept
{
    JAM_DESTROY_VERTEX_BUFFER(m_cubeVbh);
    JAM_DESTROY_INDEX_BUFFER(m_cubeIbh);
}

void PBRScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_rot += m_dRot * _deltaTime;
        m_lightTime += _deltaTime;
    }

    m_lights[0].pos    = Vec3 { Cos(m_lightTime * 0.6f) * 7.f, 4.5f, 1.5f + Sin(m_lightTime * 0.6f) * 3.f };
    m_lights[1].pos    = Vec3 { Cos(m_lightTime * 0.6f + kPi) * 7.f, 4.5f, 1.5f + Sin(m_lightTime * 0.6f + kPi) * 3.f };
    m_lights[0].lookAt = Vec3 { 0.f, 0.f, 2.f };
    m_lights[1].lookAt = Vec3 { 0.f, 0.f, 2.f };
}

void PBRScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eyePos   = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBCubeMap cubeMap;
    cubeMap.type            = AsUnderlying(m_cubeMapType);
    cubeMap.backgroundColor = m_backgroundColor;
    gfx.UpdateBuffer(m_cubeMapCbh, AsMemoryView(cubeMap));

    CBLights lights;
    for (Int32 i = 0; i < m_lights.size(); ++i)
    {
        const Light&     l     = m_lights[i];
        CBLights::Light& light = lights.lights[i];
        light.lightPos         = l.pos;
        light.lightDir         = Vec3 { l.lookAt - l.pos }.Normalized();
        light.lightColor       = l.color;
        light.fallOffStart     = l.fallOffStart;
        light.fallOffEnd       = l.fallOffEnd;
        light.innerConeCos     = Cos(l.innerConeRad);
        light.outerConeCos     = Cos(l.outerConeRad);
        light.bEnabled         = l.bEnabled ? 1 : 0;
        light.type             = AsUnderlying(l.type);
    }
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(lights));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void PBRScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("PBRScene"))
    {
        ImGui::End();
        return;
    }

    static constexpr const char* kToneMappingNames[] = {
        "None",
        "Linear",
        "Exponential",
        "Reinhard",
        "Uncharted2",
        "MGSV",
        "Uchimura",
        "Filmic",
        "ACES",
        "PBR Neutral",
        "Flim",
        "AgX"
    };

    ImGui::Checkbox("Animate", &m_bAnimate);
    ImGui::Text("35 spheres: roughness increases left to right");
    ImGui::Text("Metallic increases from the front row (0) to the back (1)");
    const char* environments[] = { "Studio", "Courtyard", "Church" };
    Int32       environment    = m_environmentIndex;
    if (ImGui::Combo("Environment / IBL", &environment, environments, IM_ARRAYSIZE(environments)))
    {
        LoadEnvironment_(environment);
    }
    const char* backgrounds[] = { "Solid color", "Diffuse irradiance", "Prefiltered specular", "Environment" };
    Int32       background    = AsUnderlying(m_cubeMapType);
    if (ImGui::Combo("Background", &background, backgrounds, IM_ARRAYSIZE(backgrounds)))
    {
        m_cubeMapType = static_cast<eCubeMap>(background);
    }
    if (m_cubeMapType == eCubeMap::None)
    {
        ImGui::ColorEdit3("Background color", &m_backgroundColor.x);
    }

    bool bChanged = false;
    bChanged |= ImGui::SliderFloat("Exposure", &m_exposure, -5.f, 5.f);
    bChanged |= ImGui::SliderFloat("Gamma", &m_gamma, 0.1f, 5.f);

    Int32 tmIndex = static_cast<Int32>(m_tmType);
    if (ImGui::Combo("Tone Mapping", &tmIndex, kToneMappingNames, IM_ARRAYSIZE(kToneMappingNames)))
    {
        m_tmType = static_cast<eToneMapping>(tmIndex);
        bChanged = true;
    }

    if (bChanged)
    {
        UpdateToneMappingParams_();
    }

    ImGui::End();
}

void PBRScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void PBRScene::LoadEnvironment_(const Int32 _index)
{
    // Release the previous maps only after removing their cached GPU bindings.
    GetGraphics().Reset();
    m_environmentIndex = Clamp(_index, 0, 2);
    if (m_environmentIndex == 2)
    {
        m_pIBLIrradiance  = LoadTexture(SCENE_PATH("IBLDiffuse.dds"));
        m_pIBLSpecular    = LoadTexture(SCENE_PATH("IBLSpecular.dds"));
        m_pIBLEnvironment = LoadTexture(SCENE_PATH("IBLEnv.dds"));
    }
    else
    {
        const String path = String::Formatted("{}Environments/{}/", SCENE_PATH(""), m_environmentIndex == 0 ? "Studio" : "Courtyard");
        m_pIBLIrradiance  = LoadTexture(path + "Diffuse.dds");
        m_pIBLSpecular    = LoadTexture(path + "Specular.dds");
        m_pIBLEnvironment = LoadTexture(path + "Environment.dds");
    }
}

void PBRScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_HDR", 1.f, eTextureFormat::RGBA16_Float, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attachments[2] = {};
    attachments[0].name         = "T_HDR";
    attachments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attachments);

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLights));
    m_graph.EmplaceConstantBuffer("CB_CubeMap", sizeof(CBCubeMap));
    m_graph.EmplaceConstantBuffer("CB_Material", sizeof(CBMaterial));

    m_entityCbh   = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh      = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh    = m_graph.GetConstantBuffer("CB_Light");
    m_cubeMapCbh  = m_graph.GetConstantBuffer("CB_CubeMap");
    m_materialCbh = m_graph.GetConstantBuffer("CB_Material");

    LoadProgram(m_graph, "P_PBR", "Resources/PBRScene/PBRShader.hlsl");
    LoadProgram(m_graph, "P_CubeMap", "Resources/PBRScene/CubeMapShader.hlsl");

    m_graph.AddRenderPass("Pass_CubeMap")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_CubeMap")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kSkyboxRenderFlags)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kAnisotropicWrap, eShader::PixelShader, kLinearWrapSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_CubeMap", eShader::PixelShader, CBCubeMap::kSlot)
        .SetExecutor(Bind<&PBRScene::SubmitCubeMap_>(this));

    m_graph.AddRenderPass("Pass_PBR")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_PBR")
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kLinearClamp, eShader::PixelShader, kLinearClampSlot)
        .SetSampler(kAnisotropicWrap, eShader::PixelShader, kLinearWrapSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLights::kSlot)
        .SetConstantBuffer("CB_Material", eShader::PixelShader, CBMaterial::kSlot)
        .SetExecutor(Bind<&PBRScene::SubmitPBR_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo tmInfo = {};
    tmInfo.filter          = eImageFilter::ToneMapping;
    tmInfo.output          = eImageFilterOutput::Final;
    tmInfo.input0Texture   = "T_HDR";
    ifm.AddImageFilter(m_graph, "Pass_ToneMapping", tmInfo);
    UpdateToneMappingParams_();
}

void PBRScene::SubmitCubeMap_() const
{
    Graphics& gfx = GetGraphics();

    gfx.SetVertexBuffer(m_cubeVbh);
    gfx.SetIndexBuffer(m_cubeIbh);
    m_pIBLIrradiance->Bind(eShader::PixelShader, kIblIrradianceSlot);
    m_pIBLSpecular->Bind(eShader::PixelShader, kIblSpecularSlot);
    m_pIBLEnvironment->Bind(eShader::PixelShader, kIblEnvironmentSlot);
    gfx.Submit();
}

void PBRScene::InitObjects_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    layout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    m_textureSets.resize(IM_ARRAYSIZE(kTextureSetNames));
    for (SizeT i = 0; i < m_textureSets.size(); ++i)
    {
        const String prefix = String::Formatted("{}{}", SCENE_PATH("Materials/"), kTextureSetNames[i]);

        m_textureSets[i].pAlbedo = LoadTexture(String::Formatted("{}_Albedo.jpg", prefix));
        m_textureSets[i].pArm    = LoadTexture(String::Formatted("{}_Arm.jpg", prefix));
        m_textureSets[i].pNormal = LoadTexture(String::Formatted("{}_Normal.jpg", prefix));
    }

    const Ref<Mesh> pSphere = CreateSphereMesh(0.58f, 4, layout);
    const Ref<Mesh> pFloor  = CreatePlaneMesh({ 28.f, 24.f }, Vec3::kUp, layout);

    SceneObject& floor = m_objects.emplace_back();
    floor.pMesh        = pFloor;
    floor.texSet       = 0;
    floor.position     = { 0.f, 0.f, 3.f };
    floor.uvScale      = { 5.f, 4.f };
    floor.metallic     = 1.f;

    // Columns vary roughness; rows vary metallic, with identical base color.
    for (Int32 row = 0; row < 5; ++row)
    {
        for (Int32 column = 0; column < 7; ++column)
        {
            SceneObject& obj = m_objects.emplace_back();
            obj.pMesh        = pSphere;
            obj.position     = { -4.5f + static_cast<float>(column) * 1.5f, 0.7f, 1.f + static_cast<float>(row) * 1.65f };
            obj.albedo       = { 0.8f, 0.5f, 0.2f };
            obj.roughness    = 0.05f + static_cast<float>(column) / 6.f * 0.95f;
            obj.metallic     = static_cast<float>(row) / 4.f;
        }
    }

    auto addModel = [&](const char* _name, const Vec3 _position, const float _height, const float _yaw) {
        const String path     = String::Formatted("{}Gallery/{}/", MODEL_PATH(""), _name);
        const Int32  material = static_cast<Int32>(m_textureSets.size());
        TextureSet&  set      = m_textureSets.emplace_back();
        set.pAlbedo           = LoadTexture(path + "Albedo.jpg");
        set.pArm              = LoadTexture(path + "Arm.jpg");
        set.pNormal           = LoadTexture(path + "NormalGL.jpg");
        SceneObject& obj      = m_objects.emplace_back();
        obj.pMesh             = LoadMesh(path + "Mesh.gltf", layout);
        obj.texSet            = material;
        obj.position          = _position;
        obj.scale             = _height;
        obj.euler.y           = _yaw;
        obj.metallic          = 1.f;
    };
    addModel("GothicStatue", { -7.1f, 0.f, 4.f }, 4.4f, ToRad(15.f));
    addModel("MarbleBust", { 7.1f, 0.f, 4.f }, 3.9f, ToRad(-15.f));
    addModel("BronzeShark", { -3.2f, 0.f, -2.8f }, 2.1f, ToRad(15.f));
    addModel("BrassVase", { 3.2f, 0.f, -2.8f }, 2.5f, ToRad(-20.f));

    const Int32 columnMaterial = static_cast<Int32>(m_textureSets.size());
    TextureSet& columnSurface  = m_textureSets.emplace_back();
    columnSurface.pAlbedo      = LoadTexture(MODEL_PATH("Gallery/Column/Albedo.jpg"));
    columnSurface.pArm         = LoadTexture(MODEL_PATH("Gallery/Column/Arm.jpg"));
    columnSurface.pNormal      = LoadTexture(MODEL_PATH("Gallery/Column/NormalGL.jpg"));
    const Ref<Mesh> pColumn    = LoadMesh(MODEL_PATH("Gallery/Column/Mesh.gltf"), layout);
    for (const float x: { -9.f, 9.f })
    {
        SceneObject& obj = m_objects.emplace_back();
        obj.pMesh        = pColumn;
        obj.texSet       = columnMaterial;
        obj.position     = { x, 0.f, 8.f };
        obj.scale        = 5.5f;
        obj.metallic     = 1.f;
        obj.uvScale      = { 2.f, 2.f };
    }
}

void PBRScene::SubmitPBR_()
{
    Graphics& gfx = GetGraphics();

    m_pIBLIrradiance->Bind(eShader::PixelShader, kIblIrradianceSlot);
    m_pIBLSpecular->Bind(eShader::PixelShader, kIblSpecularSlot);
    m_pBRDFLUT->Bind(eShader::PixelShader, kBrdfLUTSlot);

    for (const SceneObject& obj: m_objects)
    {
        CBMaterial material = {};
        material.albedo     = obj.albedo;
        material.roughness  = obj.roughness;
        material.metallic   = obj.metallic;

        if (obj.texSet >= 0)
        {
            const TextureSet& set = m_textureSets[obj.texSet];
            set.pAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
            set.pNormal->Bind(eShader::PixelShader, kNormalSlot);
            set.pArm->Bind(eShader::PixelShader, kAoRoughnessMetallicSlot);
            material.albedo = Vec3::kOne;
            material.flags  = kMaterialAlbedoMap | kMaterialNormalMap | kMaterialArmMap;
        }
        gfx.UpdateBuffer(m_materialCbh, AsMemoryView(material));

        const Vec3 euler = { obj.euler.x, obj.euler.y + m_rot * obj.spinSpeed, obj.euler.z };
        const Mtx4 world = Mtx4::MakeSRT(Vec3 { obj.scale }, euler, obj.position);

        CBEntity entity;
        entity.world         = world.Transpose();
        entity.invTransWorld = world.Inverse();
        entity.uvScale       = obj.uvScale;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

        obj.pMesh->Submit();
    }
}

void PBRScene::UpdateToneMappingParams_() const
{
    CBImageFilterParam param;
    param.fparam.x = m_exposure;               // exposure
    param.fparam.y = m_gamma;                  // gamma
    param.iparam.z = AsUnderlying(m_tmType);   // tone mapping type

    ImageFilterManager& ifm = GetImageFilterManager();
    ifm.UpdateParams(m_graph, "Pass_ToneMapping", param);
}
