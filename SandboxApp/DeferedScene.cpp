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

constexpr Int32 kAlbedoSlot            = 0;
constexpr Int32 kNormalSlot            = 1;
constexpr Int32 kDepthSlot             = 2;
constexpr Int32 kColorSlot             = 3;
constexpr Int32 kNormalSceneSlot       = 4;
constexpr Int32 kLightSlot             = 5;
constexpr Int32 kLinearWrapSamplerSlot = 1;

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
    float padding;
};

struct CBDeferedSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightPos;
    float lightRadius;
    Vec3  lightCol;
    float padding;
};

}   // namespace

DeferedScene::DeferedScene()
{
    Graphics& gfx = GetGraphics();

    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    constexpr Flags<eAssimpImporterFlags> kFlags = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;

    // dragon
    Expect<AssimpImporter> dragonImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", kFlags);
    m_pDragon                             = dragonImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pDragonAlbedo                       = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/Albedo.jpg").GetValue();
    m_pDragonNormal                       = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/NormalGL.png").GetValue();

    // bunny
    Expect<AssimpImporter> bunnyImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", kFlags);
    m_pBunny                             = bunnyImporter->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pBunnyAlbedo                       = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/Albedo.png").GetValue();
    m_pBunnyNormal                       = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/NormalGL.png").GetValue();

    m_pPlane = CreatePlaneMesh({ 10.f, 10.f }, -Vec3::kForward, attribLayout);

    // full screen triangle
    {
        constexpr Vec2 kVertices[3] = {
            { -1.f,  1.f },
            {  3.f,  1.f },
            { -1.f, -3.f }
        };
        constexpr UInt16 kIndices[3] = { 0, 1, 2 };

        VertexLayout vl = {};
        vl.Add(eVertexAttribute::Position, eAttributeFormat::Float2);

        m_fullTriVbh = gfx.CreateVertexBuffer(kVertices, vl);
        m_fullTriIbh = gfx.CreateIndexBuffer(kIndices);
    }

    // cbuffers
    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneEntity));
    m_cameraCbh = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBDeferedSceneLight));

    // shaders
    ShaderLoadInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;

    ShaderLoadInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;

    // gbuffer program
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneGBufferShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "VS compile failed: {}", vsComp.GetErrorMessage());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneGBufferShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "PS compile failed: {}", psComp.GetErrorMessage());

        m_gbufferPh = gfx.CreateProgram(gfx.CreateShader(vsComp.GetBytecode()), gfx.CreateShader(psComp.GetBytecode()), true);
    }

    // light program
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneLightingShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "VS compile failed: {}", vsComp.GetErrorMessage());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneLightingShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "PS compile failed: {}", psComp.GetErrorMessage());

        m_lightPh = gfx.CreateProgram(gfx.CreateShader(vsComp.GetBytecode()), gfx.CreateShader(psComp.GetBytecode()), true);
    }

    // combine program
    {
        ShaderCompiler vsComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneCombineShader.hlsl", vsInfo);
        JAM_ASSERT(vsComp.IsSucceeded(), "VS compile failed: {}", vsComp.GetErrorMessage());

        ShaderCompiler psComp = ShaderCompiler::CompileHLSLFile("Resources/DeferedScene/DeferedSceneCombineShader.hlsl", psInfo);
        JAM_ASSERT(psComp.IsSucceeded(), "PS compile failed: {}", psComp.GetErrorMessage());

        m_combinePh = gfx.CreateProgram(gfx.CreateShader(vsComp.GetBytecode()), gfx.CreateShader(psComp.GetBytecode()), true);
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

void DeferedScene::OnUpdate(const float _deltaSec)
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

void DeferedScene::OnRender(const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    // camera cbuffer
    {
        CBDeferedSceneCamera cbCamera = {};
        cbCamera.viewProj             = m_camera.CalcViewProj().Transpose();
        cbCamera.invViewProj          = cbCamera.viewProj.Inverse();
        cbCamera.eyePos               = m_camera.GetEyePos();
        gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(cbCamera));

        gfx.SetConstantBuffer(m_cameraCbh, eShader::VertexShader, CBDeferedSceneCamera::kSlot);
        gfx.SetConstantBuffer(m_cameraCbh, eShader::PixelShader, CBDeferedSceneCamera::kSlot);
    }

    // gbuffer pass
    {
        gfx.ClearRenderTarget(m_gbufferFbh, RGBA::kBlack, 0);
        gfx.ClearRenderTarget(m_gbufferFbh, RGBA::kBlack, 1);
        gfx.ClearDepthStencil(m_gbufferFbh, true, false);

        gfx.SetFrameBuffer(m_gbufferFbh);
        gfx.SetViewport(m_vp);
        gfx.SetProgram(m_gbufferPh);
        gfx.SetRenderFlags(kBackCullDepthTest);
        gfx.SetBlendFlags(kOpaqueBlend, 0);
        gfx.SetSampler(kLinearWrap, eShader::PixelShader, kLinearWrapSamplerSlot);
        gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBDeferedSceneEntity::kSlot);
        gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBDeferedSceneEntity::kSlot);

        const Vec3 euler = { 0.f, m_rot, 0.f };

        // dragon
        {
            constexpr Vec3 kS { 0.01f };
            constexpr Vec3 kT = { -3.f, 0.f, 0.f };

            CBDeferedSceneEntity cbEntity = {};
            cbEntity.world                = Mtx4::CreateSRT(kS, euler, kT).Transpose();
            cbEntity.invTransWorld        = Mtx4::CreateInvSRT(kS, euler, kT);
            cbEntity.bHasTexture          = 1;
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            m_pDragonAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
            m_pDragonNormal->Bind(eShader::PixelShader, kNormalSlot);
            m_pDragon->Submit();
        }

        // bunny
        {
            constexpr Vec3 kS { 0.01f };
            constexpr Vec3 kT = { 3.f, 0.f, 0.f };

            CBDeferedSceneEntity cbEntity = {};
            cbEntity.world                = Mtx4::CreateSRT(kS, euler, kT).Transpose();
            cbEntity.invTransWorld        = Mtx4::CreateInvSRT(kS, euler, kT);
            cbEntity.bHasTexture          = 1;
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            m_pBunnyAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
            m_pBunnyNormal->Bind(eShader::PixelShader, kNormalSlot);
            m_pBunny->Submit();
        }

        // plane
        {
            constexpr Vec3 kT = { 0.f, 0.f, 0.3f };

            CBDeferedSceneEntity cbEntity = {};
            cbEntity.world                = Mtx4::CreateSRT(Vec3::kOne, Vec3::kZero, kT).Transpose();
            cbEntity.invTransWorld        = Mtx4::CreateInvSRT(Vec3::kOne, Vec3::kZero, kT);
            cbEntity.bHasTexture          = 0;
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            m_pPlane->Submit();
        }
    }

    // light pass
    {
        gfx.ClearRenderTarget(m_lightFbh, RGBA::kBlack, 0);

        gfx.SetFrameBuffer(m_lightFbh);
        gfx.SetVertexBuffer(m_fullTriVbh);
        gfx.SetIndexBuffer(m_fullTriIbh);
        gfx.SetProgram(m_lightPh);
        gfx.SetRenderFlags(kBackCull);
        gfx.SetBlendFlags(kAdditiveBlend, 0);
        gfx.SetTexture(m_colorTexh, eShader::PixelShader, kColorSlot);
        gfx.SetTexture(m_normalTexh, eShader::PixelShader, kNormalSceneSlot);
        gfx.SetTexture(m_depthTexh, eShader::PixelShader, kDepthSlot);
        gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBDeferedSceneLight::kSlot);

        for (Int32 i = 0; i < m_numLights; ++i)
        {
            const Light& l = m_lights[i];

            CBDeferedSceneLight cbLight = {};
            cbLight.lightPos            = l.pos;
            cbLight.lightRadius         = l.radius;
            cbLight.lightCol            = l.color;
            gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
            gfx.Submit();
        }

        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kColorSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kNormalSceneSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDepthSlot);
    }

    // combine pass
    {
        gfx.SetFrameBuffer(kInvalidHandle);
        gfx.SetVertexBuffer(m_fullTriVbh);
        gfx.SetIndexBuffer(m_fullTriIbh);
        gfx.SetProgram(m_combinePh);
        gfx.SetBlendFlags(kOpaqueBlend, 0);
        gfx.SetTexture(m_colorTexh, eShader::PixelShader, kColorSlot);
        gfx.SetTexture(m_lightTexh, eShader::PixelShader, kLightSlot);
        gfx.Submit();

        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kColorSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kLightSlot);
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

void DeferedScene::OnResize(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_FRAME_BUFFER(m_lightFbh);

    Graphics& gfx = GetGraphics();
    m_colorTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_normalTexh  = gfx.CreateTexture2D(_width, _height, eTextureFormat::RG11B10_Float, false, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_depthTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::D24_UNorm_S8_UInt, false, 1, eTextureCreateFlags::DepthStencilAttachment);
    m_lightTexh   = gfx.CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::RenderTargetAttachment);

    Attachment attachments[3] = {};
    attachments[0].texh       = m_colorTexh;
    attachments[1].texh       = m_normalTexh;
    attachments[2].texh       = m_depthTexh;
    m_gbufferFbh              = gfx.CreateFrameBuffer(attachments, true);
    m_lightFbh                = gfx.CreateFrameBuffer(m_lightTexh, true);
    m_vp                      = { static_cast<UInt16>(_width), static_cast<UInt16>(_height) };
}

void DeferedScene::InitLights_(const Int32 _from, const Int32 _to)
{
    for (Int32 i = _from; i < _to; ++i)
    {
        Light& l = m_lights[i];
        l.radius = MakeRandomFloat(1.5f, 4.f);
        l.offset = { MakeRandomFloat(-5.f, 5.f), MakeRandomFloat(-5.f, 5.f), MakeRandomFloat(-1.f, 1.f) };
        l.pos    = l.offset;
        l.color  = { MakeRandomFloat(0.3f, 1.f), MakeRandomFloat(0.3f, 1.f), MakeRandomFloat(0.3f, 1.f) };
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