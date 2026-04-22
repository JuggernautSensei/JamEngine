#include "pch.h"
#include "ShadowMappingScene.h"

#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBShadowMappingSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world         = Mtx4::kIdentity;
    Mtx4 worldInvTrans = Mtx4::kIdentity;
};

struct CBShadowMappingSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  view    = Mtx4::kIdentity;
    Mtx4  proj    = Mtx4::kIdentity;
    Vec3  pos     = Vec3::kZero;
    float padding = 0.f;
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

void ShadowMappingScene::OnUpdate(float _dt)
{
}

void ShadowMappingScene::OnRender(const RenderView& _view)
{
    m_graph.Execute(_view.fbh, _view.vp);
}

void ShadowMappingScene::OnImGuiRender()
{
}

void ShadowMappingScene::InitMeshes_()
{
    VertexLayout vl = {};
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
    vl.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

    VertexLayout posOnlyVl = {};
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2);

    Flags<eAssimpImporterFlags> flags          = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;
    Expect<AssimpImporter>      dragonImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", flags);
    Expect<AssimpImporter>      bunnyImporter  = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", flags);

    m_entities.clear();
    m_entities.resize(4);

    m_entities[0].pMesh = dragonImporter->ConvertToModel(vl, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[1].pMesh = bunnyImporter->ConvertToModel(vl, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[2].pMesh = CreatePlaneMesh({ 10.f, 10.f }, vl);
    m_entities[3].pMesh = CreateCubeMesh({ 1.f, 1.f, 1.f }, vl);

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

    // gbuffer program
    AddProgram_VertexPixel_(
        "P_GBuffer",
        "Resources/ShadowMappingScene/ShadowMappingSceneGBufferShader.hlsl");

    // lighting program
    AddProgram_VertexPixel_(
        "P_Lighting",
        "Resources/ShadowMappingScene/ShadowMappingSceneLightDepthShader.hlsl");

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

    // lighting pass
    m_graph.AddPass("Pass_Lighting")
        .SetUseExternalOutput()
        .SetAsFinalPass()
        .SetViewportFromFrameBufferResolution()
        .SetProgram("P_Lighting")
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetTexture("T_SceneNormal", eShader::PixelShader, 0)
        .SetExecutor(Bind<&ShadowMappingScene::RenderFullScreenTri_>(this));
}

void ShadowMappingScene::RenderMeshes_()
{
    Graphics& gfx = GetGraphics();

    for (const Entity& entity: m_entities)
    {
        const Mtx4& world = entity.transform.GetSRT();

        CBShadowMappingSceneEntity cb = {};
        cb.world                      = world.Transpose();
        cb.worldInvTrans              = world.Invert();
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
