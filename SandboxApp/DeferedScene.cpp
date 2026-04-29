#include "pch.h"
#include "DeferedScene.h"

#include "Gui.h"
#include "RandomGenerator.h"
#include "RenderStates.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/Random.h"

namespace
{

constexpr Int32 kDeferedSceneAlbedoSlot      = 0;
constexpr Int32 kDeferedSceneNormalSlot      = 1;
constexpr Int32 kDeferedSceneDepthSceneSlot  = 2;
constexpr Int32 kDeferedSceneColorSceneSlot  = 3;
constexpr Int32 kDeferedSceneNormalSceneSlot = 4;
constexpr Int32 kDeferedSceneLightSceneSlot  = 5;

constexpr Int32 kDeferedSceneLinaerWrapSamplerSlot = 0;

struct CBDeferedSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    Mtx4   invTransWorld;
    UInt32 bHasTexture;
    float  padding[3];
};

struct CBDeferedSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  viewProj;
    Mtx4  invViewProj;
    Vec3  eyePos;
    float padding1;
};

struct CBDeferedSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightPos;
    float lightRadius;
    Vec3  lightCol;
    float padding1;
};

}   // namespace

DeferedScene::DeferedScene()
{
    Flags<eAssimpImporterFlags> flags          = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;
    Expect<AssimpImporter>      dragonImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", flags);
    Expect<AssimpImporter>      bunnyImporter  = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", flags);

    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    m_pDragon       = dragonImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pDragonAlbedo = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/Albedo.jpg").GetValue();
    m_pDragonNormal = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/NormalGL.png").GetValue();

    m_pBunny       = bunnyImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pBunnyAlbedo = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/Albedo.png").GetValue();
    m_pBunnyNormal = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/NormalGL.png").GetValue();

    m_pPlane = CreatePlaneMesh({ 10.f, 10.f }, -Vec3::kForward, attribLayout);

    // full screen tri

    Graphics& gfx = GetGraphics();

    Vec3 triVertices[] = {
        { -1.f,  1.f, 0.f },
        {  3.f,  1.f, 0.f },
        { -1.f, -3.f, 0.f }
    };

    VertexLayout vl = {};
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);

    UInt16 triIndices[] = { 0, 1, 2 };

    m_fullTriVbh = gfx.CreateVertexBuffer(triVertices, vl);
    m_fullTriIbh = gfx.CreateIndexBuffer(triIndices);

    // cbuffer
    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneEntity));
    m_cameraCbh = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneLight));

    // shader
    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;

    // gbuffer
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneGBufferShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "Failed to compile shader: {}", vsComp.GetErrorMessage());
        ShaderHandle vsh = gfx.CreateShader(vsComp.GetBytecode());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneGBufferShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "Failed to compile shader: {}", psComp.GetErrorMessage());
        ShaderHandle psh = gfx.CreateShader(psComp.GetBytecode());

        m_gbufferPh = gfx.CreateProgram(vsh, psh, true);
    }

    // light
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneLightingShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "Failed to compile shader: {}", vsComp.GetErrorMessage());
        ShaderHandle vsh = gfx.CreateShader(vsComp.GetBytecode());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneLightingShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "Failed to compile shader: {}", psComp.GetErrorMessage());
        ShaderHandle psh = gfx.CreateShader(psComp.GetBytecode());

        m_lightPh = gfx.CreateProgram(vsh, psh, true);
    }

    // combine
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneCombineShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "Failed to compile shader: {}", vsComp.GetErrorMessage());
        ShaderHandle vsh = gfx.CreateShader(vsComp.GetBytecode());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneCombineShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "Failed to compile shader: {}", psComp.GetErrorMessage());
        ShaderHandle psh = gfx.CreateShader(psComp.GetBytecode());

        m_combinePh = gfx.CreateProgram(vsh, psh, true);
    }

    m_camera.trans.SetTranslationZ(-15.f);
    InitLights_();
}

DeferedScene::~DeferedScene() noexcept
{
    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_FRAME_BUFFER(m_lightFbh);
    JAM_DESTROY_VERTEX_BUFFER(m_fullTriVbh);
    JAM_DESTROY_INDEX_BUFFER(m_fullTriIbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cameraCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_PROGRAM(m_gbufferPh);
    JAM_DESTROY_PROGRAM(m_lightPh);
    JAM_DESTROY_PROGRAM(m_combinePh);
}

void DeferedScene::OnUpdate(
    const float _deltaSec)
{
    m_counter += _deltaSec;
    m_camera.Update(_deltaSec);

    if (m_bAnimate)
    {
        m_rot += _deltaSec * m_dRot;
    }

    for (Int32 i = 0; i < m_numLights; ++i)
    {
        Light& l = m_lights[i];
        l.pos.x  = l.offset.x + Cos(m_counter * l.freqX + l.phaseX) * l.ampX;
        l.pos.y  = l.offset.y + Sin(m_counter * l.freqY + l.phaseY) * l.ampY;
        l.pos.z  = l.offset.z + Sin(m_counter * l.freqZ + l.phaseZ) * l.ampZ;
    }
}

void DeferedScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    gfx.ClearRenderTarget(m_gbufferFbh, { 0, 0, 0, 0 }, 0);
    gfx.ClearRenderTarget(m_gbufferFbh, { 0, 0, 0, 0 }, 1);
    gfx.ClearDepthStencil(m_gbufferFbh, true, false);

    // camera
    gfx.SetConstantBuffer(m_cameraCbh, eShader::VertexShader, CBDeferedSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cameraCbh, eShader::PixelShader, CBDeferedSceneCamera::kSlot);

    CBDeferedSceneCamera cbCamera = {};
    cbCamera.viewProj             = m_camera.CalcViewProj().Transpose();
    cbCamera.invViewProj          = cbCamera.viewProj.Inverse();
    cbCamera.eyePos               = m_camera.GetEyePos();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(cbCamera));

    // render gbuffer
    {
        gfx.SetRenderFlags(kBackCullDepthTest);

        gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 0);
        gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 1);

        gfx.SetSampler(kLinearWrap, eShader::PixelShader, kDeferedSceneLinaerWrapSamplerSlot);

        gfx.SetProgram(m_gbufferPh);
        gfx.SetViewport(m_vp);
        gfx.SetFrameBuffer(m_gbufferFbh);

        gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBDeferedSceneEntity::kSlot);
        gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBDeferedSceneEntity::kSlot);

        // dragon
        const Vec3     euler    = { 0.f, m_rot, 0.f };
        constexpr Vec3 kDragonS = Vec3 { 0.01f };
        constexpr Vec3 kDragonT = { -3.f, 0.f, 0.f };

        CBDeferedSceneEntity cbEntity = {};
        cbEntity.world                = Mtx4::CreateSRT(kDragonS, euler, kDragonT).Transpose();
        cbEntity.invTransWorld        = Mtx4::CreateInvSRT(kDragonS, euler, kDragonT);
        cbEntity.bHasTexture          = 1;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

        m_pDragonAlbedo->Bind(eShader::PixelShader, kDeferedSceneAlbedoSlot);
        m_pDragonNormal->Bind(eShader::PixelShader, kDeferedSceneNormalSlot);
        m_pDragon->Submit();

        // bunny
        constexpr Vec3 kBunnyS = Vec3 { 0.01f };
        constexpr Vec3 kBunnyT = { 3.f, 0.f, 0.f };

        cbEntity.world         = Mtx4::CreateSRT(kBunnyS, euler, kBunnyT).Transpose();
        cbEntity.invTransWorld = Mtx4::CreateInvSRT(kBunnyS, euler, kBunnyT);
        cbEntity.bHasTexture   = 1;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

        m_pBunnyAlbedo->Bind(eShader::PixelShader, kDeferedSceneAlbedoSlot);
        m_pBunnyNormal->Bind(eShader::PixelShader, kDeferedSceneNormalSlot);
        m_pBunny->Submit();

        // plane (wall)
        constexpr Vec3 kPlaneT = { 0.f, 0.f, 0.3f };

        cbEntity.world         = Mtx4::CreateSRT(Vec3::kOne, Vec3::kZero, kPlaneT).Transpose();
        cbEntity.invTransWorld = Mtx4::CreateInvSRT(Vec3::kOne, Vec3::kZero, kPlaneT);
        cbEntity.bHasTexture   = 0;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
        m_pPlane->Submit();
    }

    // render light
    {
        gfx.SetRenderFlags(kBackCull);
        gfx.SetBlendFlags(kAdditiveBlend, 0);

        gfx.ClearRenderTarget(m_lightFbh, { 0, 0, 0, 0 }, 0);
        gfx.SetFrameBuffer(m_lightFbh);
        gfx.SetVertexBuffer(m_fullTriVbh);
        gfx.SetIndexBuffer(m_fullTriIbh);
        gfx.SetProgram(m_lightPh);
        gfx.SetTexture(m_colorTexh, eShader::PixelShader, kDeferedSceneColorSceneSlot);
        gfx.SetTexture(m_normalTexh, eShader::PixelShader, kDeferedSceneNormalSceneSlot);
        gfx.SetTexture(m_depthTexh, eShader::PixelShader, kDeferedSceneDepthSceneSlot);
        gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBDeferedSceneLight::kSlot);

        for (Int32 i = 0; i < m_numLights; ++i)
        {
            const Light&        l       = m_lights[i];
            CBDeferedSceneLight cbLight = {};
            cbLight.lightPos            = l.pos;
            cbLight.lightRadius         = l.radius;
            cbLight.lightCol            = l.color;
            gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
            gfx.Submit();
        }

        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneNormalSceneSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneDepthSceneSlot);
    }

    // combine
    {
        gfx.SetBlendFlags(kOpaqueBlend, 0);
        gfx.SetFrameBuffer(kInvalidHandle);
        gfx.SetVertexBuffer(m_fullTriVbh);
        gfx.SetIndexBuffer(m_fullTriIbh);
        gfx.SetProgram(m_combinePh);
        gfx.SetTexture(m_colorTexh, eShader::PixelShader, kDeferedSceneColorSceneSlot);
        gfx.SetTexture(m_lightTexh, eShader::PixelShader, kDeferedSceneLightSceneSlot);
        gfx.Submit();

        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneColorSceneSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneLightSceneSlot);
        gfx.Touch();
    }
}

void DeferedScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("DeferedScene"))
    {
        ImGui::End();
        return;
    }

    Int32 numLights = m_numLights;
    if (ImGui::SliderInt("Light Count", &numLights, 1, kMaxLights))
    {
        const Int32 prev = m_numLights;
        m_numLights      = numLights;
        if (m_numLights > prev)
            InitLights_(prev, m_numLights);
    }

    ImGui::End();
}

void DeferedScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_FRAME_BUFFER(m_lightFbh);

    Graphics& gfx = GetGraphics();
    m_colorTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_normalTexh  = gfx.CreateTexture2D(_width, _height, eTextureFormat::RG11B10_Float, false, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_depthTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::D24_UNorm_S8_UInt, false, 1, eTextureCreateFlags::DepthStencilAttachment);
    m_lightTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::RenderTargetAttachment);

    TextureHandle gbufferAttach[] = { m_colorTexh, m_normalTexh, m_depthTexh };
    m_gbufferFbh                  = gfx.CreateFrameBuffer(gbufferAttach, true);
    m_vp                          = { static_cast<UInt16>(_width), static_cast<UInt16>(_height) };

    TextureHandle lightAttach[] = { m_lightTexh };
    m_lightFbh                  = gfx.CreateFrameBuffer(lightAttach, true);
}

void DeferedScene::InitLights_(
    const Int32 _from,
    const Int32 _to)
{
    for (Int32 i = _from; i < _to; ++i)
    {
        Light& l   = m_lights[i];
        l.radius   = MakeRandomFloat(1.5f, 4.f);
        l.offset.x = MakeRandomFloat(-5.f, 5.f);
        l.offset.y = MakeRandomFloat(-5.f, 5.f);
        l.offset.z = MakeRandomFloat(-1.f, 1.f);
        l.pos      = l.offset;
        l.color    = {
            MakeRandomFloat(0.3f, 1.f),
            MakeRandomFloat(0.3f, 1.f),
            MakeRandomFloat(0.3f, 1.f)
        };
        l.freqX  = MakeRandomFloat(0.3f, 2.f);
        l.freqY  = MakeRandomFloat(0.3f, 2.f);
        l.freqZ  = MakeRandomFloat(0.1f, 0.8f);
        l.phaseX = MakeRandomFloat(0.f, k2Pi);
        l.phaseY = MakeRandomFloat(0.f, k2Pi);
        l.phaseZ = MakeRandomFloat(0.f, k2Pi);
        l.ampX   = MakeRandomFloat(2.f, 6.f);
        l.ampY   = MakeRandomFloat(2.f, 6.f);
        l.ampZ   = MakeRandomFloat(0.2f, 1.f);
    }
}