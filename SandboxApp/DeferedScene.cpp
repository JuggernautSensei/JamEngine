#include "pch.h"
#include "DeferedScene.h"

#include "Gui.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/EditorWidgets.h"

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

    Mtx4   cb_world         = Mtx4::kIdentity;
    Mtx4   cb_worldInvTrans = Mtx4::kIdentity;
    Vec3   cb_baseColor     = Vec3::kOne;
    UInt32 cb_bHasTextures  = 0;
};

struct CBDeferedSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  cb_eyePos      = Vec3::kZero;
    float padding1       = 0.f;
    Mtx4  cb_viewProj    = Mtx4::kIdentity;
    Mtx4  cb_viewProjInv = Mtx4::kIdentity;
};

struct CBDeferedSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  cb_lightPos    = Vec3::kZero;
    float cb_lightRadius = 0.f;
    Vec3  cb_lightCol    = Vec3::kOne;
    float padding1       = 0.f;
};

struct DeferedSceneVertex
{
    Vec3 pos      = {};
    Vec2 texcoord = {};
    Vec3 normal   = {};
    Vec3 tangent  = {};

    inline static VertexLayout s_layout = [] {
        VertexLayout layout = {};
        layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        layout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
        layout.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);
        return layout;
    }();
};

}   // namespace

DeferedScene::DeferedScene()
{
    Flags<eAssimpImporterFlags> flags          = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;
    Expect<AssimpImporter>      dragonImporter = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", flags);
    Expect<AssimpImporter>      bunnyImporter  = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", flags);

    m_pDragon       = dragonImporter->ConvertToModel(DeferedSceneVertex::s_layout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pDragonAlbedo = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/Albedo.jpg").GetValue();
    m_pDragonNormal = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/NormalGL.png").GetValue();

    m_pBunny       = bunnyImporter->ConvertToModel(DeferedSceneVertex::s_layout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_pBunnyAlbedo = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/Albedo.png").GetValue();
    m_pBunnyNormal = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/NormalGL.png").GetValue();

    m_pPlane = CreatePlaneMesh({ 10.f, 10.f }, DeferedSceneVertex::s_layout);

    Vec3 triVertices[] = {
        { -1.f,  1.f, 0.f },
        {  3.f,  1.f, 0.f },
        { -1.f, -3.f, 0.f }
    };

    VertexLayout vl = {};
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
    Graphics& gfx = GetGraphics();
    m_fullTriVbh  = gfx.CreateVertexBuffer(triVertices, vl);

    UInt16 triIndices[] = {
        0, 1, 2
    };
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
    InitLights();
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
    if (m_bAnimate)
    {
        m_time += _deltaSec;
    }

    for (Int32 i = 0; i < m_numLights; ++i)
    {
        Light& l = m_lights[i];
        l.pos    = l.offset;
        l.pos.x += Cos(m_time * m_deltaLightMove + static_cast<float>(i)) * 5.f;
        l.pos.y += Sin(m_time * m_deltaLightMove + static_cast<float>(i)) * 5.f;
    }

    m_camera.Update(_deltaSec);
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

    const Mtx4 viewProj = m_camera.trans.GetSRT().Invert() * m_camera.cmr.GetProj();

    CBDeferedSceneCamera cbCamera = {};
    cbCamera.cb_eyePos            = m_camera.trans.GetTranslation();
    cbCamera.cb_viewProj          = viewProj.Transpose();
    cbCamera.cb_viewProjInv       = cbCamera.cb_viewProj.Invert();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(cbCamera));

    // render gbuffer
    {
        gfx.SetRenderFlags(
            eRenderFlags::DepthFunc_Less
            | eRenderFlags::DepthWrite
            | eRenderFlags::CullMode_Back
            | eRenderFlags::Primitive_TriangleList);

        gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 0);
        gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 1);

        gfx.SetSampler(
            eSamplerFlags::AddressU_Wrap
                | eSamplerFlags::AddressV_Wrap
                | eSamplerFlags::AddressW_Wrap
                | eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear,
            eShader::PixelShader,
            kDeferedSceneLinaerWrapSamplerSlot);

        gfx.SetProgram(m_gbufferPh);
        gfx.SetViewport(m_vp);
        gfx.SetFrameBuffer(m_gbufferFbh);

        // dragon
        {
            float                x        = -1.f;
            float                y        = Sin(m_time * m_deltaAnimation);
            float                yaw      = m_time * m_deltaAnimation;
            const Mtx4           world    = Mtx4::CreateScale(0.005f, 0.005f, 0.005f) * Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
            CBDeferedSceneEntity cbEntity = {};
            cbEntity.cb_world             = world.Transpose();
            cbEntity.cb_worldInvTrans     = world.Invert();
            cbEntity.cb_baseColor         = Vec3 { 1.f, 1.f, 1.f };
            cbEntity.cb_bHasTextures      = 1;
            gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBDeferedSceneEntity::kSlot);
            gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBDeferedSceneEntity::kSlot);
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            m_pDragonAlbedo->Bind(eShader::PixelShader, kDeferedSceneAlbedoSlot);
            m_pDragonNormal->Bind(eShader::PixelShader, kDeferedSceneNormalSlot);
            m_pDragon->Submit();
        }

        // bunny
        {
            float                x        = 1.f;
            float                y        = Sin(m_time * m_deltaAnimation + ToRad(45.f));
            float                yaw      = m_time * m_deltaAnimation + ToRad(45.f);
            const Mtx4           world    = Mtx4::CreateScale(0.005f, 0.005f, 0.005f) * Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
            CBDeferedSceneEntity cbEntity = {};
            cbEntity.cb_world             = world.Transpose();
            cbEntity.cb_worldInvTrans     = world.Invert();
            cbEntity.cb_baseColor         = Vec3 { 1.f, 1.f, 1.f };
            cbEntity.cb_bHasTextures      = 1;
            gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBDeferedSceneEntity::kSlot);
            gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBDeferedSceneEntity::kSlot);
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            m_pBunnyAlbedo->Bind(eShader::PixelShader, kDeferedSceneAlbedoSlot);
            m_pBunnyNormal->Bind(eShader::PixelShader, kDeferedSceneNormalSlot);
            m_pBunny->Submit();
        }

        // plane (wall)
        {
            const Mtx4           world    = Mtx4::CreateFromYawPitchRoll(0.f, ToRad(-90.f), 0.f) * Mtx4::CreateTranslation(0.f, 0.f, 0.3f);
            CBDeferedSceneEntity cbEntity = {};
            cbEntity.cb_world             = world.Transpose();
            cbEntity.cb_worldInvTrans     = world.Invert();
            cbEntity.cb_baseColor         = Vec3 { 1.f, 1.f, 1.f };
            cbEntity.cb_bHasTextures      = 0;
            gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBDeferedSceneEntity::kSlot);
            gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBDeferedSceneEntity::kSlot);
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
            m_pPlane->Submit();
        }
    }

    // render light
    {
        // additive blend
        gfx.SetRenderFlags(
            eRenderFlags::CullMode_Back
            | eRenderFlags::Primitive_TriangleList);

        gfx.SetBlendFlags(
            eBlendFlags::Enable
                | eBlendFlags::WriteMask_All
                | eBlendFlags::SrcBlend_One | eBlendFlags::DstBlend_One | eBlendFlags::BlendOp_Add
                | eBlendFlags::SrcBlendAlpha_SrcAlpha | eBlendFlags::DstBlendAlpha_InvSrcAlpha | eBlendFlags::BlendOpAlpha_Add,
            0);

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
            cbLight.cb_lightPos         = l.pos;
            cbLight.cb_lightRadius      = l.radius;
            cbLight.cb_lightCol         = l.color;
            gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
            gfx.Submit();
        }

        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneNormalSceneSlot);
        gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kDeferedSceneDepthSceneSlot);
    }

    // combine
    {
        gfx.SetRenderFlags(eRenderFlags::Primitive_TriangleList);
        gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 0);
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

    // 1. 성능 모니터링
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Separator();

    // 2. 씬 및 애니메이션 설정
    if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Lights", &m_bAnimate);
        ImGui::SliderFloat("Light Move Speed", &m_deltaLightMove, 0.0f, 2.0f);
        ImGui::SliderFloat("Mesh Anim Speed", &m_deltaAnimation, 0.0f, 5.0f);
    }

    // 3. 라이트 시스템 제어
    if (ImGui::CollapsingHeader("Global Light System", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Number of Lights", &m_numLights, 1, 1024);

        static float globalRadius = 2.0f;
        if (ImGui::DragFloat("Set All Radius", &globalRadius, 0.1f, 0.1f, 20.0f))
        {
            for (Int32 i = 0; i < m_numLights; ++i)
                m_lights[i].radius = globalRadius;
        }

        static Vec3 globalColor = Vec3::kOne;
        if (ImGui::ColorEdit3("Set All Color", &globalColor.x))
        {
            for (Int32 i = 0; i < m_numLights; ++i)
                m_lights[i].color = globalColor;
        }
    }

    ImGui::End();

    ImGui::Begin("Deferred Debug");

    constexpr ImVec2 kImageSize = { 200, 200 };

    ImGui::BeginGroup();
    ImGui::Text("Albedo (Color)");
    Widget_Image(m_colorTexh, kImageSize);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Normal (R11G11B10)");
    Widget_Image(m_normalTexh, kImageSize);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Depth");
    Widget_Image(m_depthTexh, kImageSize);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Light Buffer (Accumulated)");
    Widget_Image(m_lightTexh, kImageSize);
    ImGui::EndGroup();

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
    m_vp                          = { NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };

    TextureHandle lightAttach[] = { m_lightTexh };
    m_lightFbh                  = gfx.CreateFrameBuffer(lightAttach, true);
}

void DeferedScene::InitLights()
{
    for (Int32 i = 0; i < m_numLights; ++i)
    {
        Light& l   = m_lights[i];
        l.radius   = 2.f;
        l.offset.x = MakeRandom(-5.f, 5.f);
        l.offset.y = MakeRandom(-5.f, 5.f);
        l.offset.z = -0.5f;
        l.pos      = l.offset;
        l.color    = Vec3 { 1.f, 1.f, 1.f };
    }
}