#include "pch.h"
#include "PBRScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/AssimpImporter.h"

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

    Mtx4 world;
    Mtx4 invTransWorld;
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
    float  padding[3];
};

}   // namespace

PBRScene::PBRScene()
{
    Graphics& gfx = GetGraphics();

    // load model
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);
    m_pMesh = LoadMesh(SCENE_PATH("scene.gltf"), attribLayout);

    // load texture
    m_pAlbedo              = LoadTexture(SCENE_PATH("Albedo.jpeg"));
    m_pAoRoughnessMetallic = LoadTexture(SCENE_PATH("AoRoughnessMetaliic.png"));
    m_pEmissive            = LoadTexture(SCENE_PATH("Emissive.jpeg"));
    m_pNormal              = LoadTexture(SCENE_PATH("Normal.jpeg"));
    m_pIBLIrradiance       = LoadTexture(SCENE_PATH("IBLDiffuse.dds"));
    m_pIBLSpecular         = LoadTexture(SCENE_PATH("IBLSpecular.dds"));
    m_pIBLEnvironment      = LoadTexture(SCENE_PATH("IBLEnv.dds"));
    m_pBRDFLUT             = LoadTexture(SCENE_PATH("IBLBrdf.dds"));

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

    InitRenderGraph_();

    m_camera.camera.SetAspectRatio(16.f / 9.f);
    m_camera.transform.SetTranslationZ(-5.f);

    m_lights[0].bEnabled   = true;
    m_lights[0].type       = eLight::Point;
    m_lights[0].pos        = Vec3(2.f, 2.f, -2.f);
    m_lights[0].fallOffEnd = 50.f;
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
    }
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
    cubeMap.type = AsUnderlying(m_cubeMapType);
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

    m_entityCbh  = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh     = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh   = m_graph.GetConstantBuffer("CB_Light");
    m_cubeMapCbh = m_graph.GetConstantBuffer("CB_CubeMap");

    LoadProgram(m_graph, "P_PBR", "Resources/PBRScene/PBRShader.hlsl");
    LoadProgram(m_graph, "P_CubeMap", "Resources/PBRScene/CubeMapShader.hlsl");

    m_graph.AddRenderPass("Pass_CubeMap")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_CubeMap")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kSkyboxRenderFlags)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kLinearWrap, eShader::PixelShader, kLinearWrapSlot)
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
        .SetSampler(kLinearWrap, eShader::PixelShader, kLinearWrapSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLights::kSlot)
        .SetExecutor(Bind<&PBRScene::SubmitPBR_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc tmDesc = {};
    tmDesc.filter          = eImageFilter::ToneMapping;
    tmDesc.output          = eImageFilterOutput::Final;
    tmDesc.input0Texture   = "T_HDR";
    ifm.AddImageFilter(m_graph, "Pass_ToneMapping", tmDesc);
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

void PBRScene::SubmitPBR_()
{
    Graphics& gfx = GetGraphics();

    m_pIBLIrradiance->Bind(eShader::PixelShader, kIblIrradianceSlot);
    m_pIBLSpecular->Bind(eShader::PixelShader, kIblSpecularSlot);
    m_pBRDFLUT->Bind(eShader::PixelShader, kBrdfLUTSlot);
    m_pAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
    m_pNormal->Bind(eShader::PixelShader, kNormalSlot);
    m_pAoRoughnessMetallic->Bind(eShader::PixelShader, kAoRoughnessMetallicSlot);
    m_pEmissive->Bind(eShader::PixelShader, kEmissiveSlot);

    CBEntity entity;
    entity.world         = Mtx4::FromEuler({ ToRad(-90.f), m_rot, 0.f }).Transpose();
    entity.invTransWorld = entity.world;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

    m_pMesh->Submit();
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
