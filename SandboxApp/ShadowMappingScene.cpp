#include "pch.h"
#include "ShadowMappingScene.h"

#include "Gui.h"
#include "RandomGenerator.h"
#include "RenderStates.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/Random.h"

namespace
{

constexpr Int32 kShadowCascadeLevels = 3;

constexpr Int32 kTSceneNormalSlot = 0;
constexpr Int32 kTSceneColorSlot  = 1;
constexpr Int32 kTSceneDepthSlot  = 2;
constexpr Int32 kTSceneLightSlot  = 3;
constexpr Int32 kTAlbedoSlot      = 4;

constexpr Int32 kTShadowMapSlot    = 5;
constexpr Int32 kTShadowMapAdvSlot = 8;

constexpr Int32 kTImageFilterInput0 = 0;

constexpr Int32 kSLinearClampSlot      = 0;
constexpr Int32 kSLinearWrapSlot       = 1;
constexpr Int32 kSShadowComparisonSlot = 2;

struct CBShadowMappingSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    UInt32 rgba;
    UInt32 bUseTexture;
    float  padding[2];
};

struct CBShadowMappingSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  viewProj;
    Mtx4  invViewProj;
    Vec3  eyePos;
    float padding;
};

struct CBShadowMappingSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3   lightPos;
    float  lightFallOffStart;
    Vec3   lightDir;
    float  lightFallOffEnd;
    float  lightSpotInnerCos;
    float  lightSpotOuterCos;
    UInt32 bMakeShadow;
    float  padding;
};

struct CBShadowMappingSceneShadow
{
    static constexpr Int32 kSlot = 3;

    UInt32 shadowType;
    UInt32 bUseComparisonSampler;
    float  shadowBias;
    float  shadowNormalOffset;
    float  esmExponential;
    float  padding[3];
};

struct CBShadowMappingSceneShadowCast
{
    static constexpr Int32 kSlot = 4;

    Mtx4 cb_lightView;
    Mtx4 cb_lightProj;
};

struct CBShadowMappingSceneOmniShadowCast
{
    static constexpr Int32 kSlot = 5;

    Mtx4 cb_lightCube[6];
};

struct CBShadowMappingSceneCascadeShadowCast
{
    static constexpr Int32 kSlot = 6;

    Mtx4  cb_lightCascade[kShadowCascadeLevels];
    float cb_cascadeSplits[kShadowCascadeLevels];
};

}   // namespace

ShadowMappingScene::ShadowMappingScene()
{
    m_spotLight.bEnabled = true;

    InitMeshes_();
    InitRenderGraphResource_();
    BuildRenderGraph_();

    m_spotLight.pos.y      = 3.f;
    constexpr Vec3 kFocus  = Vec3 { 1.f, 0.f, 5.f };
    m_spotLight.dir        = (kFocus - m_spotLight.pos).Normalized();
    m_spotLight.fallOffEnd = 100.f;

    m_camera.trans.SetTranslation(Vec3 { -5.f, 5.f, -5.f });
    m_camera.trans.SetLookAt(kFocus, Vec3::kUp);
}

ShadowMappingScene::~ShadowMappingScene() noexcept
{
}

void ShadowMappingScene::OnUpdate(const float _dt)
{
    m_camera.Update(_dt);

    if (m_bAnimate)
    {
        for (Entity& entity: m_entities)
        {
            if (!entity.bNotAnimated)
            {
                entity.transform.AddRotationYaw(_dt * m_dRot);
            }
        }
    }
}

void ShadowMappingScene::OnRender(const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneCamera cbCamera = {};
    cbCamera.viewProj                   = m_camera.CalcViewProj().Transpose();
    cbCamera.invViewProj                = cbCamera.viewProj.Inverse();
    cbCamera.eyePos                     = m_camera.GetEyePos();
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

    ImGui::Checkbox("animate", &m_bAnimate);
    ImGui::SameLine();
    ImGui::SliderFloat("rot speed", &m_dRot, 0.f, ToRad(360.f), "%.2f rad/s");

    bool bNeedRebuildGraph = false;

    ////////// Spot Light //////////

    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bNeedRebuildGraph |= ImGui::Checkbox("enabled##spot", &m_spotLight.bEnabled);
        ImGui::SameLine();
        bNeedRebuildGraph |= ImGui::Checkbox("cast shadow##spot", &m_spotLight.bCastShadow);

        ImGui::BeginDisabled(!m_spotLight.bEnabled);

        ImGui::DragFloat3("position##spot", &m_spotLight.pos.x, 0.1f);
        ImGui::DragFloat3("direction##spot", &m_spotLight.dir.x, 0.01f, -1.f, 1.f);
        ImGui::DragFloat("fall off start##spot", &m_spotLight.fallOffStart, 0.1f, 0.f, m_spotLight.fallOffEnd);
        ImGui::DragFloat("fall off end##spot", &m_spotLight.fallOffEnd, 0.1f, m_spotLight.fallOffStart, FLT_MAX);

        if (ImGui::SliderAngle("inner cone##spot", &m_spotLight.innerConeRad, 1.f, 89.f))
        {
            m_spotLight.innerConeRad = Min(m_spotLight.innerConeRad, m_spotLight.outerConeRad - ToRad(1.f));
        }

        if (ImGui::SliderAngle("outer cone##spot", &m_spotLight.outerConeRad, 1.f, 89.f))
        {
            m_spotLight.outerConeRad = Max(m_spotLight.outerConeRad, m_spotLight.innerConeRad + ToRad(1.f));
        }

        ImGui::EndDisabled();
    }

    ////////// Point Light //////////

    if (ImGui::CollapsingHeader("Point Light"))
    {
        bNeedRebuildGraph |= ImGui::Checkbox("enabled##omni", &m_omniLight.bEnabled);

        ImGui::BeginDisabled(!m_omniLight.bEnabled);

        ImGui::DragFloat3("position##omni", &m_omniLight.pos.x, 0.1f);
        ImGui::DragFloat("fall off start##omni", &m_omniLight.fallOffStart, 0.1f, 0.f, m_omniLight.fallOffEnd);
        ImGui::DragFloat("fall off end##omni", &m_omniLight.fallOffEnd, 0.1f, m_omniLight.fallOffStart, FLT_MAX);

        ImGui::BeginDisabled();
        bool bNotImpl = false;
        ImGui::Checkbox("cast shadow (미구현)##omni", &bNotImpl);
        ImGui::EndDisabled();

        ImGui::EndDisabled();
    }

    ////////// Directional Light //////////

    if (ImGui::CollapsingHeader("Directional Light"))
    {
        bNeedRebuildGraph |= ImGui::Checkbox("enabled##dir", &m_dirLight.bEnabled);

        ImGui::BeginDisabled(!m_dirLight.bEnabled);

        ImGui::DragFloat3("direction##dir", &m_dirLight.dir.x, 0.01f, -1.f, 1.f);

        ImGui::BeginDisabled();
        bool bNotImpl = false;
        ImGui::Checkbox("cast shadow (미구현)##dir", &bNotImpl);
        ImGui::EndDisabled();

        ImGui::EndDisabled();
    }

    ////////// Shadow //////////

    if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const bool bAnyShadow = m_spotLight.bCastShadow || m_dirLight.bCastShadow || m_omniLight.bCastShadow;
        ImGui::BeginDisabled(!bAnyShadow);

        {
            int sampling = static_cast<int>(m_shadowSampling);

            bNeedRebuildGraph |= ImGui::RadioButton("Hard", &sampling, static_cast<int>(eShadowSampling::Hard));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF3", &sampling, static_cast<int>(eShadowSampling::PCF3));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF5", &sampling, static_cast<int>(eShadowSampling::PCF5));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF7", &sampling, static_cast<int>(eShadowSampling::PCF7));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("VSM", &sampling, static_cast<int>(eShadowSampling::VSM));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("ESM", &sampling, static_cast<int>(eShadowSampling::ESM));

            m_shadowSampling = static_cast<eShadowSampling>(sampling);
        }

        const bool bIsHard = m_shadowSampling == eShadowSampling::Hard;
        const bool bIsESM  = m_shadowSampling == eShadowSampling::ESM;
        const bool bIsVSM  = m_shadowSampling == eShadowSampling::VSM;

        ImGui::BeginDisabled(bIsHard || bIsVSM || bIsESM);
        ImGui::Checkbox("use comparison sampler", &m_bUseComparisonSampler);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(bIsHard);
        ImGui::DragFloat("bias", &m_shadowBias, 0.0001f, 0.f, 0.1f, "%.4f");
        ImGui::DragFloat("normal offset", &m_shadowNormalOffset, 0.0001f, 0.f, 1.f, "%.4f");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!bIsESM);
        ImGui::DragFloat("ESM exponent", &m_esmExponent, 1.f, 1.f, 500.f, "%.1f");
        ImGui::EndDisabled();

        ImGui::EndDisabled();   // bAnyShadow
    }

    if (bNeedRebuildGraph)
    {
        BuildRenderGraph_();
    }

    ImGui::End();
}

void ShadowMappingScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}

void ShadowMappingScene::InitMeshes_()
{
    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

    Flags<eAssimpImporterFlags> flags = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame;

    Expect<AssimpImporter> dragonImporterOrNot = AssimpImporter::ImportFromFile("Resources/Models/StanfordDragonPBR/StanfordDragonPBR.gltf", flags);
    JAM_ASSERT(dragonImporterOrNot, "Failed to load model.");
    Expect<AssimpImporter> bunnyImporterOrNot = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", flags);
    JAM_ASSERT(bunnyImporterOrNot, "Failed to load model.");

    // dragon
    m_entities[0].pMesh          = dragonImporterOrNot->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[0].pTextureOrNull = Texture::LoadFromFile("Resources/Models/StanfordDragonPBR/Albedo.jpg").GetValue();
    m_entities[0].transform.SetScale(Vec3 { 0.01f });
    m_entities[0].transform.SetTranslationX(1.5f);
    m_entities[0].transform.SetTranslationZ(5.f);

    // bunny
    m_entities[1].pMesh          = bunnyImporterOrNot->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_entities[1].pTextureOrNull = Texture::LoadFromFile("Resources/Models/StanfordBunnyPBR/Albedo.png").GetValue();
    m_entities[1].transform.SetScale(Vec3 { 0.01f });
    m_entities[1].transform.SetTranslationX(-1.5f);
    m_entities[1].transform.SetTranslationZ(5.f);

    // plane
    m_entities[2].pMesh          = CreatePlaneMesh({ 100.f, 100.f }, { 0, 0 }, { 100.f, 100.f }, Vec3::kUp, attribLayout);
    m_entities[2].pTextureOrNull = Texture::LoadFromFile("Resources/Textures/TileAlbedo.png").GetValue();
    m_entities[2].transform.SetTranslationY(0.f);
    m_entities[2].bNotAnimated = true;

    // cubes
    Ref<Mesh> pCube = CreateCubeMesh({ 1.f, 1.f, 1.f }, attribLayout);

    RngMWC rng {};
    for (int i = 3; i < kTotalEntities; ++i)
    {
        Entity& entity = m_entities[i];
        entity.pMesh   = pCube;
        entity.transform.SetTranslationX(MakeRandomFloat(-25.f, 25.f));
        entity.transform.SetTranslationZ(MakeRandomFloat(-25.f, 25.f));
        entity.transform.SetRotation(MakeRandomVec3(-k2Pi, k2Pi, -k2Pi, k2Pi, -k2Pi, k2Pi));
        entity.rgba = MakeRandomOpaqueRGBAFromHSV(0.f, k2Pi, 1.f, 1.f, 1.f, 1.f);
    }

    // full screen triangle
    {
        Vec2 positions[] = {
            { -1.f,  1.f },
            {  3.f,  1.f },
            { -1.f, -3.f }
        };
        UInt16 indices[] = { 0, 1, 2 };
        m_pFsTri         = Mesh::Load(positions, indices);
    }

    m_pCone   = CreateConeMesh(1.f, 1.f, -Vec3::kForward, 32, eCreateConeMesh::OriginAtApex, attribLayout);
    m_pSphere = CreateSphereMesh(1.f, 5, attribLayout);
}

void ShadowMappingScene::InitRenderGraphResource_()
{
    m_graph.AddTexture2D("T_SceneNormal", 1.f, eTextureFormat::RGBA16_Float, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, 1, eTextureCreateFlags::DepthStencilAttachment);
    m_graph.AddTexture2D("T_SceneLight", 1.f, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_ShadowMap", 1024, 1024, eTextureFormat::D32_Float, 1, eTextureCreateFlags::DepthStencilAttachment);
    m_graph.AddTexture2D("T_ShadowMapArray", 1024, 1024, eTextureFormat::D32_Float, kShadowCascadeLevels, eTextureCreateFlags::DepthStencilAttachment);
    m_graph.AddTextureCube("T_ShadowMapCube", 1024, 1024, eTextureFormat::D32_Float, 1, eTextureCreateFlags::DepthStencilAttachment);
    m_graph.AddTexture2D("T_ShadowMapAdv", 1024, 1024, eTextureFormat::RG32_Float, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_ShadowMapAdvTmp", 1024, 1024, eTextureFormat::RG32_Float, 1, eTextureCreateFlags::RenderTargetAttachment);

    m_graph.AddConstantBuffer("CB_Entity", sizeof(CBShadowMappingSceneEntity));
    m_graph.AddConstantBuffer("CB_Camera", sizeof(CBShadowMappingSceneCamera));
    m_graph.AddConstantBuffer("CB_Light", sizeof(CBShadowMappingSceneLight));
    m_graph.AddConstantBuffer("CB_Shadow", sizeof(CBShadowMappingSceneShadow));
    m_graph.AddConstantBuffer("CB_ShadowCast", sizeof(CBShadowMappingSceneShadowCast));
    m_graph.AddConstantBuffer("CB_OmniShadowCast", sizeof(CBShadowMappingSceneOmniShadowCast));
    m_graph.AddConstantBuffer("CB_CascadeShadowCast", sizeof(CBShadowMappingSceneCascadeShadowCast));

    m_entityCbh            = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh               = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh             = m_graph.GetConstantBuffer("CB_Light");
    m_shadowCbh            = m_graph.GetConstantBuffer("CB_Shadow");
    m_shadowCastCbh        = m_graph.GetConstantBuffer("CB_ShadowCast");
    m_omniShadowCastCbh    = m_graph.GetConstantBuffer("CB_OmniShadowCast");
    m_cascadeShadowCastCbh = m_graph.GetConstantBuffer("CB_CascadeShadowCast");

    StringView gbufferAttachments[] = { "T_SceneNormal", "T_SceneColor", "T_SceneDepth" };
    m_graph.AddFrameBuffer("FB_GBuffer", gbufferAttachments);
    m_graph.AddFrameBuffer("FB_Light", "T_SceneLight");
    m_graph.AddFrameBuffer("FB_ShadowMap", "T_ShadowMap");
    m_graph.AddFrameBuffer("FB_ShadowMapArray", "T_ShadowMapArray");
    m_graph.AddFrameBuffer("FB_ShadowMapCube", "T_ShadowMapCube");

    StringView shadowMapAdvAttachments[] = { "T_ShadowMapAdv", "T_ShadowMap" };
    m_graph.AddFrameBuffer("FB_ShadowMapAdv", shadowMapAdvAttachments);
    m_graph.AddFrameBuffer("FB_ShadowMapAdvTmp", "T_ShadowMapAdvTmp");

    AddProgram_VertexPixel_("P_GBuffer", "Resources/ShadowMappingScene/ShadowMappingSceneGBufferShader.hlsl");
    AddProgram_VertexPixel_("P_Combine", "Resources/ShadowMappingScene/ShadowMappingSceneCombineShader.hlsl");
    AddProgram_VertexPixel_("P_SpotShadowCast", "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowCastShader.hlsl");
    AddProgram_VertexPixel_Macro_("P_SpotShadowCastVSM", "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowCastShader.hlsl", "SHADOW_CAST_VSM", "");
    AddProgram_VertexPixel_Macro_("P_SpotShadowCastESM", "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowCastShader.hlsl", "SHADOW_CAST_VSM", "");
    AddProgram_VertexPixel_("P_SpotShadowReceive", "Resources/ShadowMappingScene/ShadowMappingSceneSpotShadowReceiveShader.hlsl");
    AddProgram_VertexPixel_Macro_("P_GaussianBlurHorizontal", "Resources/Common/ImageFilter/GaussianBlurShader.hlsl", "GAUSSIAN_BLUR_DIRECTION", "GAUSSIAN_BLUR_HORIZONTAL");
    AddProgram_VertexPixel_Macro_("P_GaussianBlurVertical", "Resources/Common/ImageFilter/GaussianBlurShader.hlsl", "GAUSSIAN_BLUR_DIRECTION", "GAUSSIAN_BLUR_VERTICAL");

    // TODO: Omni/Directional shadow 셰이더 (미구현)
    //   AddProgram_VertexPixel_("P_OmniShadowCast", "Resources/ShadowMappingScene/ShadowMappingSceneOmniShadowCastShader.hlsl");
    //   AddProgram_VertexPixel_("P_OmniShadowReceive", "Resources/ShadowMappingScene/ShadowMappingSceneOmniShadowReceiveShader.hlsl");
    //   AddProgram_VertexPixel_("P_DirectionalShadowCast", "Resources/ShadowMappingScene/ShadowMappingSceneDirectionalShadowCastShader.hlsl");
    //   AddProgram_VertexPixel_("P_DirectionalShadowReceive", "Resources/ShadowMappingScene/ShadowMappingSceneDirectionalShadowReceiveShader.hlsl");
}

void ShadowMappingScene::BuildRenderGraph_()
{
    m_graph.Clear(eRenderGraphClearFlags::Pass);

    m_graph.AddGraphicsPass("Pass_GBuffer")
        .SetFrameBuffer("FB_GBuffer")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 1)
        .SetDepthClearCommand(1.f)
        .SetProgram("P_GBuffer")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBShadowMappingSceneEntity::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBShadowMappingSceneEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBShadowMappingSceneCamera::kSlot)
        .SetSampler(kLinearWrap, eShader::PixelShader, kSLinearWrapSlot)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&ShadowMappingScene::RenderScene_>(this));

    m_graph.AddGraphicsPass("Pass_PrepareLightBuffer")
        .SetFrameBuffer("FB_Light")
        .SetRenderTargetClearCommand(RGBA::kBlack, 0);

    if (m_spotLight.bCastShadow || m_dirLight.bCastShadow || m_omniLight.bCastShadow)
    {
        m_graph.AddUtilityPass("Pass_PrepareShadow")
            .SetExecutor(Bind<&ShadowMappingScene::PrepareShadowPass_>(this));
    }

    if (m_spotLight.bEnabled)
    {
        if (m_spotLight.bCastShadow)
        {
            const StringView castProgram = m_shadowSampling == eShadowSampling::VSM ? "P_SpotShadowCastVSM"
                                         : m_shadowSampling == eShadowSampling::ESM ? "P_SpotShadowCastESM"
                                                                                    : "P_SpotShadowCast";

            const bool       bIsAdvSampling  = m_shadowSampling == eShadowSampling::VSM || m_shadowSampling == eShadowSampling::ESM;
            const StringView castFramebuffer = bIsAdvSampling ? "FB_ShadowMapAdv" : "FB_ShadowMap";

            m_graph.AddGraphicsPass("Pass_SpotShadowCast")
                .SetFrameBuffer(castFramebuffer)
                .SetDepthClearCommand(1.f)
                .SetProgram(castProgram)
                .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBShadowMappingSceneEntity::kSlot)
                .SetConstantBuffer("CB_ShadowCast", eShader::VertexShader, CBShadowMappingSceneShadowCast::kSlot)
                .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadowMappingSceneShadow::kSlot)
                .SetRenderFlags(kBackCullDepthTest)
                .SetBlendFlags(kOpaqueBlend, 0)
                .SetExecutor(Bind<&ShadowMappingScene::RenderSceneSpotShadowCast_>(this));

            if (bIsAdvSampling)
            {
                m_graph.ModifyGrphicsPass("Pass_SpotShadowCast")
                    .SetRenderTargetClearCommand(RGBA::kWhite, 0);

                m_graph.AddGraphicsPass("Pass_SpotShadowBlurHorizontal")
                    .SetFrameBuffer("FB_ShadowMapAdvTmp")
                    .SetProgram("P_GaussianBlurHorizontal")
                    .SetTexture("T_ShadowMapAdv", eShader::PixelShader, kTImageFilterInput0)
                    .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                    .SetRenderFlags(eRenderFlags::None)
                    .SetExecutor(Bind<&ShadowMappingScene::RenderFullScreenTri_>(this));

                m_graph.AddGraphicsPass("Pass_SpotShadowBlurVertical")
                    .SetFrameBuffer("FB_ShadowMapAdv")
                    .SetProgram("P_GaussianBlurVertical")
                    .SetTexture("T_ShadowMapAdvTmp", eShader::PixelShader, kTImageFilterInput0)
                    .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                    .SetRenderFlags(eRenderFlags::None)
                    .SetExecutor(Bind<&ShadowMappingScene::RenderFullScreenTri_>(this));
            }
        }

        m_graph.AddGraphicsPass("Pass_SpotShadowReceive")
            .SetFrameBuffer("FB_Light")
            .SetProgram("P_SpotShadowReceive")
            .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBShadowMappingSceneEntity::kSlot)
            .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBShadowMappingSceneCamera::kSlot)
            .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBShadowMappingSceneCamera::kSlot)
            .SetConstantBuffer("CB_Light", eShader::PixelShader, CBShadowMappingSceneLight::kSlot)
            .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadowMappingSceneShadow::kSlot)
            .SetConstantBuffer("CB_ShadowCast", eShader::PixelShader, CBShadowMappingSceneShadowCast::kSlot)
            .SetTexture("T_SceneNormal", eShader::PixelShader, kTSceneNormalSlot)
            .SetTexture("T_SceneColor", eShader::PixelShader, kTSceneColorSlot)
            .SetTexture("T_SceneDepth", eShader::PixelShader, kTSceneDepthSlot)
            .SetTexture("T_ShadowMap", eShader::PixelShader, kTShadowMapSlot)
            .SetTexture("T_ShadowMapAdv", eShader::PixelShader, kTShadowMapAdvSlot)
            .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
            .SetSampler(kShadowComparison, eShader::PixelShader, kSShadowComparisonSlot)
            .SetRenderFlags(kShadowCast)
            .SetBlendFlags(kAdditiveBlend, 0)
            .SetExecutor(Bind<&ShadowMappingScene::RenderSpotLightVolumes_>(this));
    }

    m_graph.AddGraphicsPass("Pass_Combine")
        .SetAsFinalPass()
        .SetUseExternalOutput()
        .SetProgram("P_Combine")
        .SetTexture("T_SceneColor", eShader::PixelShader, kTSceneColorSlot)
        .SetTexture("T_SceneLight", eShader::PixelShader, kTSceneLightSlot)
        .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&ShadowMappingScene::RenderFullScreenTri_>(this));
}

void ShadowMappingScene::RenderScene_()
{
    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneEntity cb = {};
    for (const Entity& entity: m_entities)
    {
        cb.world       = entity.transform.GetSRT().Transpose();
        cb.rgba        = entity.rgba.rgba;
        cb.bUseTexture = !!entity.pTextureOrNull;

        if (entity.pTextureOrNull)
        {
            entity.pTextureOrNull->Bind(eShader::PixelShader, kTAlbedoSlot);
        }

        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));
        entity.pMesh->Submit();
    }
}

void ShadowMappingScene::RenderSceneSpotShadowCast_()
{
    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneShadowCast cbShadowCast = {};

    const Vec3 dir            = m_spotLight.dir.Normalized();
    cbShadowCast.cb_lightView = Mtx4::CreateInvSRT(Vec3::kOne, dir, Vec3::kUp, m_spotLight.pos).Transpose();
    cbShadowCast.cb_lightProj = Mtx4::CreatePerspectiveFov(m_spotLight.outerConeRad * 2.f, 1.f, 0.1f, m_spotLight.fallOffEnd).Transpose();
    gfx.UpdateBuffer(m_shadowCastCbh, AsMemoryView(cbShadowCast));

    CBShadowMappingSceneEntity cbEntity = {};
    for (const Entity& entity: m_entities)
    {
        cbEntity.world = entity.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
        entity.pMesh->Submit(true);
    }
}

void ShadowMappingScene::RenderSpotLightVolumes_() const
{
    if (!m_spotLight.bEnabled)
    {
        return;
    }

    Graphics&   gfx = GetGraphics();
    const float h   = m_spotLight.fallOffEnd;
    const float r   = h * Tan(m_spotLight.outerConeRad);
    const Vec3  dir = m_spotLight.dir.Normalized();

    CBShadowMappingSceneEntity cbEntity = {};
    cbEntity.world                      = Mtx4::CreateSRT({ r, r, h }, dir, Vec3::kUp, m_spotLight.pos).Transpose();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

    CBShadowMappingSceneLight cbLight = {};
    cbLight.lightPos                  = m_spotLight.pos;
    cbLight.lightDir                  = dir;
    cbLight.lightFallOffStart         = m_spotLight.fallOffStart;
    cbLight.lightFallOffEnd           = m_spotLight.fallOffEnd;
    cbLight.lightSpotInnerCos         = Cos(m_spotLight.innerConeRad);
    cbLight.lightSpotOuterCos         = Cos(m_spotLight.outerConeRad);
    cbLight.bMakeShadow               = m_spotLight.bCastShadow;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));

    m_pCone->Submit();
}

void ShadowMappingScene::RenderPointLightVolumes_() const
{
    if (!m_omniLight.bEnabled)
    {
        return;
    }

    Graphics& gfx = GetGraphics();

    CBShadowMappingSceneEntity cb = {};
    cb.world                      = Mtx4::CreateSRT(Vec3 { m_omniLight.fallOffEnd }, Vec3::kZero, m_omniLight.pos).Transpose();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));
    m_pSphere->Submit();
}

void ShadowMappingScene::RenderDirectionalLightVolumes_() const
{
    if (!m_dirLight.bEnabled)
    {
        return;
    }

    m_pFsTri->Submit();
}

void ShadowMappingScene::RenderFullScreenTri_() const
{
    m_pFsTri->Submit();
}

void ShadowMappingScene::PrepareShadowPass_() const
{
    CBShadowMappingSceneShadow cbShadow = {};
    cbShadow.shadowType                 = UnderlyingFrom(m_shadowSampling);
    cbShadow.bUseComparisonSampler      = m_bUseComparisonSampler;
    cbShadow.shadowBias                 = m_shadowBias;
    cbShadow.shadowNormalOffset         = m_shadowNormalOffset;
    cbShadow.esmExponential             = m_esmExponent;

    Graphics& gfx = GetGraphics();
    gfx.UpdateBuffer(m_shadowCbh, AsMemoryView(cbShadow));
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

    m_graph.AddShader(String::Formatted("VS_{}", _name), vsc.GetBytecode());
    m_graph.AddShader(String::Formatted("PS_{}", _name), psc.GetBytecode());
    m_graph.AddProgram(_name, String::Formatted("VS_{}", _name), String::Formatted("PS_{}", _name));
}

void ShadowMappingScene::AddProgram_VertexPixelGeometry_(
    const StringView _name,
    const StringView _path)
{
    constexpr ShaderCompilerInfo kVsInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kPsInfo = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kGsInfo = { eShader::GeometryShader, "GSMain", {}, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, kVsInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, kPsInfo);
    const ShaderCompiler gsc = ShaderCompiler::CompileHLSLFile(_path, kGsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    JAM_ASSERT(gsc.IsSucceeded(), "Geometry shader compilation failed: {}", gsc.GetErrorMessage());

    m_graph.AddShader(String::Formatted("VS_{}", _name), vsc.GetBytecode());
    m_graph.AddShader(String::Formatted("PS_{}", _name), psc.GetBytecode());
    m_graph.AddShader(String::Formatted("GS_{}", _name), gsc.GetBytecode());
    m_graph.AddProgram(_name, String::Formatted("VS_{}", _name), String::Formatted("PS_{}", _name), String::Formatted("GS_{}", _name), {}, {});
}

void ShadowMappingScene::AddProgram_VertexPixel_Macro_(
    const StringView _name,
    const StringView _path,
    const StringView _macroDef,
    const StringView _macroValue)
{
    ShaderMacro macros[] = {
        { _macroDef, _macroValue }
    };

    const ShaderCompilerInfo vsInfo = { eShader::VertexShader, "VSMain", macros, eShaderCompileFlags::Default };
    const ShaderCompilerInfo psInfo = { eShader::PixelShader, "PSMain", macros, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, vsInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, psInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());

    m_graph.AddShader(String::Formatted("VS_{}", _name), vsc.GetBytecode());
    m_graph.AddShader(String::Formatted("PS_{}", _name), psc.GetBytecode());
    m_graph.AddProgram(_name, String::Formatted("VS_{}", _name), String::Formatted("PS_{}", _name));
}
