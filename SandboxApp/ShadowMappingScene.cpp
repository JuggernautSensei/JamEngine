#include "pch.h"
#include "ShadowMappingScene.h"

#include "Gui.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/Random.h"

namespace
{

constexpr Int32 kShadowCascadeLevels = 3;
constexpr Int32 kNumLightMatricse    = (6 > kShadowCascadeLevels ? 6 : kShadowCascadeLevels);

constexpr Int32 kTSceneNormalSlot = 0;
constexpr Int32 kTSceneColorSlot  = 1;
constexpr Int32 kTSceneDepthSlot  = 2;

struct CBShadowMappingSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world;
    Mtx4 worldInvTrans;
};

struct CBShadowMappingSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  view;
    Mtx4  proj;
    Vec3  eyePos;
    float padding;
};

struct CBShadowMappingSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3   cb_lightPos;
    float  cb_lightFallOffStart;
    Vec3   cb_lightDir;   // use dir light
    float  cb_lightFallOffEnd;
    float  cb_lightSpotInnerCos;
    float  cb_lightSpotOuterCos;
    UInt32 cb_makeShadow;
    float  cb_shadowBias;
};

struct CBShadowCaster
{
    Mtx4 cb_light[kNumLightMatricse];
};

}   // namespace

ShadowMappingScene::ShadowMappingScene()
{
    InitMeshes_();
    InitRenderGraph_();
}

ShadowMappingScene::~ShadowMappingScene() noexcept
{
}

void ShadowMappingScene::OnUpdate(const float _dt)
{
    m_camera.Update(_dt);

    for (Entity& entity: m_entities)
    {
        entity.transform.AddRotationYaw(_dt * m_dRot);
    }
}

void ShadowMappingScene::OnRender(const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneCamera cbCamera = {};
    cbCamera.view                       = m_camera.CalcView();
    cbCamera.proj                       = m_camera.GetProj();
    cbCamera.eyePos                     = _view.eye;
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));

    m_graph.Execute(_view.fbh, _view.vp);
}

void ShadowMappingScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("ShadowMappingScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::End();
}

void ShadowMappingScene::InitMeshes_()
{
    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

    Flags<eAssimpImporterFlags> flags          = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;
    Expect<AssimpImporter>      dragonImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", flags);
    Expect<AssimpImporter>      bunnyImporter  = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", flags);

    m_entities.resize(4);

    RngMWC rng {};

    m_entities[0].pMesh = dragonImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[0].transform.SetScale(Vec3 { 0.1f });
    m_entities[0].transform.SetTranslationX(GenerateRandomFloat(rng, -10.f, 10.f));
    m_entities[0].transform.SetTranslationZ(GenerateRandomFloat(rng, -10.f, 10.f));

    m_entities[1].pMesh = bunnyImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[1].transform.SetScale(Vec3 { 0.1f });
    m_entities[1].transform.SetTranslationX(GenerateRandomFloat(rng, -10.f, 10.f));
    m_entities[1].transform.SetTranslationZ(GenerateRandomFloat(rng, -10.f, 10.f));

    m_entities[2].pMesh = CreatePlaneMesh({ 10.f, 10.f }, attribLayout);
    m_entities[2].transform.SetTranslationY(-1.5f);

    m_entities[3].pMesh = CreateCubeMesh({ 1.f, 1.f, 1.f }, attribLayout);
    m_entities[3].transform.SetTranslationX(GenerateRandomFloat(rng, -10.f, 10.f));
    m_entities[3].transform.SetTranslationZ(GenerateRandomFloat(rng, -10.f, 10.f));

    // full screen triangle
    {
        Vec3 positions[] = {
            { -1.f, 1.f, 0.f },
            {  3.f, 1.f, 0.f },
            { -1.f, 3.f, 0.f }
        };

        UInt16 indices[] = {
            0, 1, 2
        };

        Graphics& gfx = GetGraphics();
        m_pFsTri      = Mesh::Load(positions, indices);
    }
}

void ShadowMappingScene::InitRenderGraph_()
{
    // textures
    m_graph.AddTexture2D("T_SceneNormal", 1.f, eTextureFormat::RG11B10_Float, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_SceneLight", 1.f, eTextureFormat::R16_Float, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_SceneDepth", 1.f, eTextureFormat::D24_UNorm_S8_UInt, 1, eTextureCreateFlags::DepthStencilAttachment);

    // cbuffer
    m_graph.AddConstantBuffer("CB_Entity", sizeof(CBShadowMappingSceneEntity));
    m_graph.AddConstantBuffer("CB_Camera", sizeof(CBShadowMappingSceneCamera));
    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");

    // frame buffer
    StringView gbufferAttachments[] = { "T_SceneNormal", "T_SceneDepth" };
    m_graph.AddFrameBuffer("FB_GBuffer", gbufferAttachments);

    AddProgram_VertexPixel_(
        "P_GBuffer",
        "Resources/ShadowMappingScene/ShadowMappingSceneGBufferShader.hlsl");

    AddProgram_VertexPixel_(
        "P_SpotShadowCast",
        "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowCastShader.hlsl");

    AddProgram_VertexPixel_(
        "P_SpotShadowReceive",
        "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowReceiveShader.hlsl");

    // gbuffer pass
    m_graph.AddPass("Pass_GBuffer")
        .SetFrameBuffer("FB_GBuffer")
        .SetViewportFromFrameBufferResolution()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 1)
        .SetDepthClearCommand(1.f)
        .SetProgram("P_GBuffer")
        .SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite)
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBShadowMappingSceneEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBShadowMappingSceneCamera::kSlot)
        .SetExecutor(Bind<&ShadowMappingScene::RenderMeshes_>(this));
}

void ShadowMappingScene::RenderMeshes_()
{
    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneEntity cb = {};
    for (const Entity& entity: m_entities)
    {
        cb.world         = entity.transform.GetSRT().Transpose();
        cb.worldInvTrans = entity.transform.CalcInvSRT();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));
        entity.pMesh->Submit();
    }
}

void ShadowMappingScene::RenderLightVolumes_() const
{
}

void ShadowMappingScene::RenderFullScreenTri_() const
{
    m_pFsTri->Submit();
}

void ShadowMappingScene::AddProgram_VertexPixel_(
    const StringView _name,
    const StringView _path)
{
    constexpr ShaderCompilerInfo kVsInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kPsInfo = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, kVsInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, kPsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());

    const String vsName = String::Formatted("VS_{}", _name);
    const String psName = String::Formatted("PS_{}", _name);

    m_graph.AddShader(vsName, vsc.GetBytecode());
    m_graph.AddShader(psName, psc.GetBytecode());
    m_graph.AddProgram(_name, vsName, psName);
}
