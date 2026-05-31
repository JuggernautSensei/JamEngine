#include "pch.h"
#include "ShadowMappingScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RandomGenerator.h"
#include "RenderStates.h"
#include "Utils.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/Filter.h"
#include "../JamEngine/Random.h"

#define SCENE_PATH(_filepath) "Resources/ShadowMappingScene/" _filepath

namespace
{

constexpr Int32 kShadowCascadeLevels = 3;

constexpr Int32 kTSceneNormalSlot = 0;
constexpr Int32 kTSceneColorSlot  = 1;
constexpr Int32 kTSceneDepthSlot  = 2;
constexpr Int32 kTSceneLightSlot  = 3;
constexpr Int32 kTAlbedoSlot      = 4;

constexpr Int32 kTShadowMapSlot        = 5;
constexpr Int32 kTShadowMapAdvSlot     = 6;
constexpr Int32 kTOmniShadowMapSlot    = 7;
constexpr Int32 kTOmniShadowMapAdvSlot = 8;

constexpr Int32 kTShadowMapAdvSATRWSlot       = 0;
constexpr Int32 kTShadowMapAdvSATColCarrySlot = 1;
constexpr Int32 kTShadowMapAdvSATRowCarrySlot = 2;

constexpr Int32 kTImageFilterInput0 = 0;

constexpr Int32 kSLinearClampSlot      = 0;
constexpr Int32 kSLinearWrapSlot       = 1;
constexpr Int32 kSShadowComparisonSlot = 2;

constexpr StringView kOmniShadowMapFBNames[]           = { "FB_OmniShadowMap_PosX", "FB_OmniShadowMap_NegX", "FB_OmniShadowMap_PosY", "FB_OmniShadowMap_NegY", "FB_OmniShadowMap_PosZ", "FB_OmniShadowMap_NegZ" };
constexpr StringView kOmniShadowMapAdvFBNames[]        = { "FB_OmniShadowMapAdv_PosX", "FB_OmniShadowMapAdv_NegX", "FB_OmniShadowMapAdv_PosY", "FB_OmniShadowMapAdv_NegY", "FB_OmniShadowMapAdv_PosZ", "FB_OmniShadowMapAdv_NegZ" };
constexpr StringView kOmniShadowCasterPassNames[]      = { "Pass_OmniShadowCaster_PosX", "Pass_OmniShadowCaster_NegX", "Pass_OmniShadowCaster_PosY", "Pass_OmniShadowCaster_NegY", "Pass_OmniShadowCaster_PosZ", "Pass_OmniShadowCaster_NegZ" };
constexpr StringView kOmniShadowMapAdvBlurXPassNames[] = { "Pass_OmniShadowMapAdvBlurX_PosX", "Pass_OmniShadowMapAdvBlurX_NegX", "Pass_OmniShadowMapAdvBlurX_PosY", "Pass_OmniShadowMapAdvBlurX_NegY", "Pass_OmniShadowMapAdvBlurX_PosZ", "Pass_OmniShadowMapAdvBlurX_NegZ" };
constexpr StringView kOmniShadowMapAdvBlurYPassNames[] = { "Pass_OmniShadowMapAdvBlurY_PosX", "Pass_OmniShadowMapAdvBlurY_NegX", "Pass_OmniShadowMapAdvBlurY_PosY", "Pass_OmniShadowMapAdvBlurY_NegY", "Pass_OmniShadowMapAdvBlurY_PosZ", "Pass_OmniShadowMapAdvBlurY_NegZ" };
constexpr StringView kOmniShadowMapAdvAAPassNames[]    = { "Pass_OmniShadowMapAdvAA_PosX", "Pass_OmniShadowMapAdvAA_NegX", "Pass_OmniShadowMapAdvAA_PosY", "Pass_OmniShadowMapAdvAA_NegY", "Pass_OmniShadowMapAdvAA_PosZ", "Pass_OmniShadowMapAdvAA_NegZ" };

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    UInt32 rgba;
    UInt32 bUseTexture;
    float  padding[2];
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  viewProj;
    Mtx4  invViewProj;
    Vec3  eyePos;
    float padding;
};

struct CBLight
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

struct CBShadow
{
    static constexpr Int32 kSlot = 3;

    UInt32 shadowType;
    UInt32 bUseComparisonSampler;
    float  shadowBias;
    float  shadowNormalOffset;
    float  esmExponential;
    Vec4   casecadeSplits;
};

struct CBShadowCaster
{
    static constexpr Int32 kSlot = 4;

    Mtx4 lightView;
    Mtx4 lightProj;
};

}   // namespace

ShadowMappingScene::ShadowMappingScene()
{
    InitMeshes_();
    InitRenderGraphResources_();
    InitRenderGraphShadowTextures_();
    InitRenderGraphPasses_();

    m_spotLight.pos.y      = 3.f;
    constexpr Vec3 kFocus  = Vec3 { 1.f, 0.f, 5.f };
    m_spotLight.dir        = (kFocus - m_spotLight.pos).Normalized();
    m_spotLight.fallOffEnd = 100.f;

    m_omniLight.pos.y        = 3.f;
    m_omniLight.fallOffStart = 10.f;
    m_omniLight.fallOffEnd   = 30.f;

    m_camera.transform.SetTranslation(Vec3 { -5.f, 5.f, -5.f });
    m_camera.transform.SetLookAt(kFocus, Vec3::kUp);
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

    CBCamera camera;
    camera.viewProj    = m_camera.CalcViewProj().Transpose();
    camera.invViewProj = camera.viewProj.Inverse();
    camera.eyePos      = m_camera.GetEyePos();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    m_graph.Execute(_view.fbh, _view.vp);
}

void ShadowMappingScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("ShadowMappingScene"))
    {
        ImGui::End();
        return;
    }

    bool bNeedRebuildGraph           = false;
    bool bNeedRecreateShadowTextures = false;

    ////////// Animation //////////

    ImGui::Checkbox("animate", &m_bAnimate);
    ImGui::SameLine();
    ImGui::SliderFloat("rot speed", &m_dRot, 0.f, ToRad(360.f), "%.2f rad/s");

    ////////// Active Light //////////

    {
        int activeLight = static_cast<int>(m_activeLight);
        bNeedRebuildGraph |= ImGui::RadioButton("None", &activeLight, static_cast<int>(eLight::None));
        ImGui::SameLine();
        bNeedRebuildGraph |= ImGui::RadioButton("Spot", &activeLight, static_cast<int>(eLight::Spot));
        ImGui::SameLine();
        bNeedRebuildGraph |= ImGui::RadioButton("Omni", &activeLight, static_cast<int>(eLight::Omni));
        ImGui::SameLine();
        bNeedRebuildGraph |= ImGui::RadioButton("Directional", &activeLight, static_cast<int>(eLight::Directional));
        m_activeLight = static_cast<eLight>(activeLight);
    }

    ////////// Light Settings //////////

    if (m_activeLight == eLight::Spot)
    {
        if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bNeedRebuildGraph |= ImGui::Checkbox("cast shadow##spot", &m_spotLight.bCastShadow);

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
        }
    }
    else if (m_activeLight == eLight::Omni)
    {
        if (ImGui::CollapsingHeader("Omni Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bNeedRebuildGraph |= ImGui::Checkbox("cast shadow##spot", &m_omniLight.bCastShadow);

            ImGui::DragFloat3("position##omni", &m_omniLight.pos.x, 0.1f);
            ImGui::DragFloat("fall off start##omni", &m_omniLight.fallOffStart, 0.1f, 0.f, m_omniLight.fallOffEnd);
            ImGui::DragFloat("fall off end##omni", &m_omniLight.fallOffEnd, 0.1f, m_omniLight.fallOffStart, FLT_MAX);
        }
    }
    else if (m_activeLight == eLight::Directional)
    {
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("direction##dir", &m_dirLight.dir.x, 0.01f, -1.f, 1.f);
            ImGui::TextDisabled("cast shadow (미구현)");
        }
    }

    ////////// Shadow //////////

    if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // shadow map texture settings
        {
            constexpr const char* kResolutions[] = { "256 x 256", "512 x 512", "1024 x 1024", "2048 x 2048", "4096 x 4096" };
            const int             min            = static_cast<int>(eShadowMapSize::x256);
            const int             max            = static_cast<int>(eShadowMapSize::x4096);
            const char*           display        = kResolutions[static_cast<int>(m_shadowMapSize)];
            if (ImGui::SliderInt("resolution", reinterpret_cast<int*>(&m_shadowMapSize), min, max, display))
            {
                bNeedRecreateShadowTextures = true;
            }
        }

        ImGui::Separator();

        // shadow filtering
        {
            int sampling = static_cast<int>(m_shadowFilter);
            bNeedRebuildGraph |= ImGui::RadioButton("Hard", &sampling, static_cast<int>(eShadowFiltering::Hard));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF3", &sampling, static_cast<int>(eShadowFiltering::PCF3));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF5", &sampling, static_cast<int>(eShadowFiltering::PCF5));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF7", &sampling, static_cast<int>(eShadowFiltering::PCF7));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("VSM", &sampling, static_cast<int>(eShadowFiltering::VSM));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("ESM", &sampling, static_cast<int>(eShadowFiltering::ESM));
            m_shadowFilter = static_cast<eShadowFiltering>(sampling);
        }

        const bool bIsHard         = m_shadowFilter == eShadowFiltering::Hard;
        const bool bIsVSM          = m_shadowFilter == eShadowFiltering::VSM;
        const bool bIsESM          = m_shadowFilter == eShadowFiltering::ESM;
        const bool bIsAdvFiltering = bIsVSM || bIsESM;

        // comparison sampler
        ImGui::BeginDisabled(bIsHard || bIsAdvFiltering);
        ImGui::Checkbox("use comparison sampler", &m_bUseComparisonSampler);
        ImGui::EndDisabled();

        // post-process
        ImGui::BeginDisabled(!bIsAdvFiltering);
        bNeedRebuildGraph |= ImGui::Checkbox("apply anti-aliasing", &m_bApplyAA);
        bNeedRebuildGraph |= ImGui::Checkbox("apply blur", &m_bApplyBlur);
        ImGui::EndDisabled();
        ImGui::Separator();

        // bias / normal offset
        ImGui::BeginDisabled(bIsHard);
        ImGui::DragFloat("bias", &m_shadowBias, 0.0001f, 0.f, 0.1f, "%.4f");
        ImGui::DragFloat("normal offset", &m_shadowNormalOffset, 0.0001f, 0.f, 1.f, "%.4f");
        ImGui::EndDisabled();

        // ESM exponent
        ImGui::BeginDisabled(!bIsESM);
        ImGui::DragFloat("ESM exponent", &m_esmExponent, 1.f, 1.f, 500.f, "%.1f");
        ImGui::EndDisabled();
    }

    ////////// 그래프 재빌드 / 텍스처 재생성 //////////

    // 텍스처 재생성이 필요하면 패스도 함께 재빌드
    if (bNeedRecreateShadowTextures)
    {
        InitRenderGraphShadowTextures_();
        InitRenderGraphPasses_();
    }
    else if (bNeedRebuildGraph)
    {
        InitRenderGraphPasses_();
    }

    ImGui::End();
}

void ShadowMappingScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.camera.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}

void ShadowMappingScene::InitMeshes_()
{
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    // dragon
    m_entities[0].pMesh          = LoadMesh(MODEL_PATH("StanfordDragonPBR/StanfordDragonPBR.gltf"), attribLayout);
    m_entities[0].pTextureOrNull = LoadTexture(MODEL_PATH("StanfordDragonPBR/Albedo.jpg"));
    m_entities[0].transform.SetScale(Vec3 { 0.01f });
    m_entities[0].transform.SetTranslationX(1.5f);
    m_entities[0].transform.SetTranslationZ(5.f);

    // bunny
    m_entities[1].pMesh          = LoadMesh(MODEL_PATH("StanfordBunnyPBR/StanfordBunnyPBR.gltf"), attribLayout);
    m_entities[1].pTextureOrNull = LoadTexture(MODEL_PATH("StanfordBunnyPBR/Albedo.png"));
    m_entities[1].transform.SetScale(Vec3 { 0.01f });
    m_entities[1].transform.SetTranslationX(-1.5f);
    m_entities[1].transform.SetTranslationZ(5.f);

    // plane
    m_entities[2].pMesh          = CreatePlaneMesh({ 100.f, 100.f }, { 0, 0 }, { 100.f, 100.f }, Vec3::kUp, attribLayout);
    m_entities[2].pTextureOrNull = LoadTexture(TEXTURE_PATH("TileAlbedo.png"));
    m_entities[2].transform.SetTranslationY(0.f);
    m_entities[2].bNotAnimated = true;

    // cube (for navigation)
    m_pCube = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);

    // entity mesh
    Ref<Mesh> pEntityMesh = CreateCubeMesh(Vec3::kOne, attribLayout);

    RngMWC rng {};
    for (int i = 3; i < kTotalEntities; ++i)
    {
        Entity& entity = m_entities[i];
        entity.pMesh   = pEntityMesh;
        entity.transform.SetTranslationX(MakeRandomFloat(-25.f, 25.f));
        entity.transform.SetTranslationZ(MakeRandomFloat(-25.f, 25.f));
        entity.transform.SetRotation(MakeRandomVec3(-k2Pi, k2Pi, -k2Pi, k2Pi, -k2Pi, k2Pi));
        entity.rgba = MakeRandomOpaqueRGBAFromHSV(0.f, k2Pi, 1.f, 1.f, 1.f, 1.f);
    }

    m_pCone   = CreateConeMesh(1.f, 1.f, -Vec3::kForward, 32, eCreateConeMesh::OriginAtApex, attribLayout);
    m_pSphere = CreateSphereMesh(1.f, 5, attribLayout);
}

void ShadowMappingScene::InitRenderGraphResources_()
{
    Graphics& gfx = GetGraphics();

    m_graph.EmplaceTexture2D("T_SceneNormal", 1.f, eTextureFormat::RGBA16_Float, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceTexture2D("T_SceneLight", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneFinal", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLight));
    m_graph.EmplaceConstantBuffer("CB_Shadow", sizeof(CBShadow));
    m_graph.EmplaceConstantBuffer("CB_ShadowCaster", sizeof(CBShadowCaster));
    m_graph.EmplaceConstantBuffer("CB_GaussianFilterKernel", sizeof(CBImageFilterKernel));
    m_graph.EmplaceConstantBuffer("CB_CubeMapIndexing", sizeof(CBImageFilterParam));

    m_entityCbh          = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh             = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh           = m_graph.GetConstantBuffer("CB_Light");
    m_shadowCbh          = m_graph.GetConstantBuffer("CB_Shadow");
    m_shadowCasterCbh    = m_graph.GetConstantBuffer("CB_ShadowCaster");
    m_cubeMapIndexingCbh = m_graph.GetConstantBuffer("CB_CubeMapIndexing");

    // upload filter
    CBImageFilterKernel kernel;
    CalcGaussianFilter1D(Span { kernel.weights, 7 }, 2.0f);
    kernel.length = 7;
    gfx.UpdateBuffer(m_graph.GetConstantBuffer("CB_GaussianFilterKernel"), AsMemoryView(kernel));

    // light
    {
        m_graph.EmplaceFrameBuffer("FB_Light", "T_SceneLight");
    }

    // gbuffer
    {
        AttachmentRG attachments[3] = {};
        attachments[0].name         = "T_SceneNormal";
        attachments[1].name         = "T_SceneColor";
        attachments[2].name         = "T_SceneDepth";
        m_graph.EmplaceFrameBuffer("FB_GBuffer", attachments);
    }

    // scene final
    {
        AttachmentRG attachments[2] = {};
        attachments[0].name         = "T_SceneFinal";
        attachments[1].name         = "T_SceneDepth";
        m_graph.EmplaceFrameBuffer("FB_SceneFinal", attachments);
    }

    LoadProgram(m_graph, "P_GBuffer", SCENE_PATH("GBufferShader.hlsl"));

    // spot light
    LoadProgram(m_graph, "P_SpotShadowCaster", SCENE_PATH("ShadowCasterShader.hlsl"));
    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "SHADOW_CAST_VSM";
        LoadProgram(m_graph, "P_SpotShadowCasterVSM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }

    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "SHADOW_CAST_ESM";
        LoadProgram(m_graph, "P_SpotShadowCasterESM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }
    LoadProgram(m_graph, "P_SpotShadowReceiver", SCENE_PATH("SpotShadowReceiverShader.hlsl"));

    // omni light
    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "OMNI_SHADOW_CAST";
        LoadProgram(m_graph, "P_OmniShadowCaster", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }
    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "SHADOW_CAST_VSM";
        LoadProgram(m_graph, "P_OmniShadowCasterVSM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }
    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "SHADOW_CAST_ESM";
        LoadProgram(m_graph, "P_OmniShadowCasterESM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }
    LoadProgram(m_graph, "P_OmniShadowReceiver", SCENE_PATH("OmniShadowReceiverShader.hlsl"));

    // misc
    LoadProgram(m_graph, "P_DebugLightCube", SCENE_PATH("DebugLightCubeShader.hlsl"));
}

void ShadowMappingScene::InitRenderGraphShadowTextures_()
{
    const Int32 shadowMapLength = GetShadowMapLength_();

    ////////// SPOT LIGHT //////////

    // shadow map
    m_graph.EmplaceTexture2D("T_ShadowMap", shadowMapLength, shadowMapLength, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceFrameBuffer("FB_ShadowMap", "T_ShadowMap");

    // advanced filtering
    m_graph.EmplaceTexture2D("T_ShadowMapAdv", shadowMapLength, shadowMapLength, eTextureFormat::RG32_Float, false, 1, eTextureFlags::RenderTarget);
    {
        AttachmentRG attahments[2] = {};
        attahments[0].name         = "T_ShadowMapAdv";
        attahments[1].name         = "T_ShadowMap";
        m_graph.EmplaceFrameBuffer("FB_ShadowMapAdv", attahments);
    }

    ////////// POINT LIGHT //////////
    // why do not use texture cube? -> for post-process blur and MSAA (for vsm)

    // shadow map
    m_graph.EmplaceTexture2D("T_OmniShadowMap", shadowMapLength, shadowMapLength, eTextureFormat::D32_Float, false, 6, eTextureFlags::DepthStencil);

    for (int i = 0; i < 6; ++i)
    {
        AttachmentRG attachments[1] = {};
        attachments[0].name         = "T_OmniShadowMap";
        attachments[0].layerOffset  = i;
        m_graph.EmplaceFrameBuffer(kOmniShadowMapFBNames[i], attachments);
    }

    // advanced filtering
    m_graph.EmplaceTexture2D("T_OmniShadowMapAdv", shadowMapLength, shadowMapLength, eTextureFormat::RG32_Float, false, 6, eTextureFlags::RenderTarget);
    for (int i = 0; i < 6; ++i)
    {
        AttachmentRG attachments[2] = {};
        attachments[0].name         = "T_OmniShadowMapAdv";
        attachments[0].layerOffset  = i;
        attachments[1].name         = "T_OmniShadowMap";
        attachments[1].layerOffset  = i;
        m_graph.EmplaceFrameBuffer(kOmniShadowMapAdvFBNames[i], attachments);
    }
}

void ShadowMappingScene::InitRenderGraphPasses_()
{
    m_graph.Clear(eRenderGraphClearFlags::Pass);

    // gbuffer pass
    m_graph.AddRenderPass("Pass_GBuffer")
        .SetFrameBuffer("FB_GBuffer")
        .SetRenderTargetClearCommand(RGBA::kBlack, 0)
        .SetRenderTargetClearCommand(RGBA::kBlack, 1)
        .SetDepthClearCommand(1.f)
        .SetProgram("P_GBuffer")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetSampler(kLinearWrap, eShader::PixelShader, kSLinearWrapSlot)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&ShadowMappingScene::RenderScene_>(this));

    // light prepare pass
    m_graph.AddRenderPass("Pass_PrepareLight")
        .SetFrameBuffer("FB_Light")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.1f, 0.1f, 0.1f), 0);

    // shadow prepare pass
    if (m_spotLight.bCastShadow || m_dirLight.bCastShadow || m_omniLight.bCastShadow)
    {
        m_graph.AddUtilityPass("Pass_PrepareShadow")
            .SetExecutor(Bind<&ShadowMappingScene::PrepareShadowPass_>(this));
    }

    const bool bAdvFiltering = m_shadowFilter == eShadowFiltering::VSM || m_shadowFilter == eShadowFiltering::ESM;

    // spot light pass
    if (m_activeLight == eLight::Spot)
    {
        String shadowMap = {};

        // shadow cast
        if (m_spotLight.bCastShadow)
        {
            const StringView program = m_shadowFilter == eShadowFiltering::VSM ? "P_SpotShadowCasterVSM"
                                     : m_shadowFilter == eShadowFiltering::ESM ? "P_SpotShadowCasterESM"
                                                                               : "P_SpotShadowCaster";

            RenderGraph::RenderPassBuilder builder =
                m_graph.AddRenderPass("Pass_SpotShadowCaster")
                    .SetProgram(program)
                    .SetDepthClearCommand(1.f)
                    .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
                    .SetConstantBuffer("CB_ShadowCaster", eShader::VertexShader, CBShadowCaster::kSlot)
                    .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadow::kSlot)
                    .SetRenderFlags(kBackCullDepthTest)
                    .SetBlendFlags(kOpaqueBlend, 0)
                    .SetExecutor(Bind<&ShadowMappingScene::RenderSceneSpotShadowCast_>(this));

            if (bAdvFiltering)
            {
                builder.SetFrameBuffer("FB_ShadowMapAdv")
                    .SetRenderTargetClearCommand(RGBA::kWhite, 0);
                shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
            }
            else
            {
                builder.SetFrameBuffer("FB_ShadowMap");
                shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), kDepthStencilSlot);
            }

            if (bAdvFiltering)
            {
                // anti aliasing
                if (m_bApplyAA)
                {
                    ImageFilterManager& ifm = GetImageFilterManager();

                    ImageFilterDesc aaDesc = {};
                    aaDesc.filter          = eImageFilter::FXAA;
                    aaDesc.output          = eImageFilterOutput::Create;
                    aaDesc.format          = eTextureFormat::RG32_Float;
                    aaDesc.width           = GetShadowMapLength_();
                    aaDesc.height          = GetShadowMapLength_();
                    aaDesc.input0Texture   = shadowMap;
                    ifm.AddImageFilter(m_graph, "Pass_SpotShadowAdvAA", aaDesc);

                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                }

                // blur
                if (m_bApplyBlur)
                {
                    ImageFilterManager& ifm = GetImageFilterManager();

                    ImageFilterDesc blurX      = {};
                    blurX.filter               = eImageFilter::Convolution1DX;
                    blurX.output               = eImageFilterOutput::Create;
                    blurX.format               = eTextureFormat::RG32_Float;
                    blurX.width                = GetShadowMapLength_();
                    blurX.height               = GetShadowMapLength_();
                    blurX.input0Texture        = shadowMap;
                    blurX.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";
                    ifm.AddImageFilter(m_graph, "Pass_SpotShadowAdvBlurX", blurX);

                    ImageFilterDesc blurY      = {};
                    blurY.filter               = eImageFilter::Convolution1DY;
                    blurY.output               = eImageFilterOutput::Inject;
                    blurY.frameBuffer          = "FB_ShadowMapAdv";
                    blurY.input0Texture        = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                    blurY.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";
                    ifm.AddImageFilter(m_graph, "Pass_SpotShadowAdvBlurY", blurY);

                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                }
            }
        }

        // lighting
        RenderGraph::RenderPassBuilder builder =
            m_graph.AddRenderPass("Pass_SpotShadowReceiver")
                .SetFrameBuffer("FB_Light")
                .SetProgram("P_SpotShadowReceiver")
                .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
                .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
                .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
                .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
                .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadow::kSlot)
                .SetConstantBuffer("CB_ShadowCaster", eShader::PixelShader, CBShadowCaster::kSlot)
                .SetTexture("T_SceneNormal", eShader::PixelShader, kTSceneNormalSlot)
                .SetTexture("T_SceneColor", eShader::PixelShader, kTSceneColorSlot)
                .SetTexture("T_SceneDepth", eShader::PixelShader, kTSceneDepthSlot)
                .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                .SetSampler(kShadowComparison, eShader::PixelShader, kSShadowComparisonSlot)
                .SetRenderFlags(kFrontCull)
                .SetBlendFlags(kAdditiveBlend, 0)
                .SetExecutor(Bind<&ShadowMappingScene::RenderSpotLightVolumes_>(this));

        if (m_spotLight.bCastShadow)
        {
            JAM_ASSERT(!shadowMap.IsEmpty(), "shadow map should have been set if shadow casting is enabled");
            builder.SetTexture(shadowMap, eShader::PixelShader, bAdvFiltering ? kTShadowMapAdvSlot : kTShadowMapSlot);
        }
    }
    else if (m_activeLight == eLight::Omni)
    {
        String shadowMap = {};

        // shadow cast
        if (m_omniLight.bCastShadow)
        {
            const StringView program = m_shadowFilter == eShadowFiltering::VSM ? "P_OmniShadowCasterVSM"
                                     : m_shadowFilter == eShadowFiltering::ESM ? "P_OmniShadowCasterESM"
                                                                               : "P_OmniShadowCaster";
            for (int i = 0; i < 6; ++i)
            {
                RenderGraph::RenderPassBuilder builder =
                    m_graph.AddRenderPass(kOmniShadowCasterPassNames[i])
                        .SetProgram(program)
                        .SetDepthClearCommand(1.f)
                        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
                        .SetConstantBuffer("CB_ShadowCaster", eShader::VertexShader, CBShadowCaster::kSlot)
                        .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadow::kSlot)
                        .SetRenderFlags(kBackCullDepthTest)
                        .SetBlendFlags(kOpaqueBlend, 0)
                        .SetExecutor(Bind([this, i] { RenderSceneOmniShadowCast_(RangesOf<eCubeFace>()[i]); }));

                if (bAdvFiltering)
                {
                    builder.SetFrameBuffer(kOmniShadowMapAdvFBNames[i])
                        .SetRenderTargetClearCommand(RGBA::kWhite, 0);
                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                }
                else
                {
                    builder.SetFrameBuffer(kOmniShadowMapFBNames[i]);
                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), kDepthStencilSlot);
                }

                if (bAdvFiltering)
                {
                    ImageFilterManager& ifm = GetImageFilterManager();
                    Graphics&           gfx = GetGraphics();

                    // anti aliasing
                    if (m_bApplyAA)
                    {
                        ImageFilterDesc aaDesc          = {};
                        aaDesc.filter                   = eImageFilter::FXAA;
                        aaDesc.output                   = eImageFilterOutput::Create;
                        aaDesc.format                   = eTextureFormat::RG32_Float;
                        aaDesc.width                    = GetShadowMapLength_();
                        aaDesc.height                   = GetShadowMapLength_();
                        aaDesc.input0Texture            = shadowMap;
                        aaDesc.bUseTexture2DArrayInput0 = true;
                        aaDesc.paramCBufferOrEmpty      = "CB_CubeMapIndexing";
                        aaDesc.executorOrNull           = Bind([this, i] { RenderScreenTriWithCubeMapIndexing_(i); });
                        ifm.AddImageFilter(m_graph, kOmniShadowMapAdvAAPassNames[i], aaDesc);

                        shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                    }

                    // blur
                    if (m_bApplyBlur)
                    {
                        ImageFilterDesc blurX      = {};
                        blurX.filter               = eImageFilter::Convolution1DX;
                        blurX.output               = eImageFilterOutput::Create;
                        blurX.format               = eTextureFormat::RG32_Float;
                        blurX.width                = GetShadowMapLength_();
                        blurX.height               = GetShadowMapLength_();
                        blurX.input0Texture        = shadowMap;
                        blurX.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";
                        if (!m_bApplyAA)
                        {
                            blurX.paramCBufferOrEmpty      = "CB_CubeMapIndexing";
                            blurX.bUseTexture2DArrayInput0 = true;
                            blurX.executorOrNull           = Bind([this, i] { RenderScreenTriWithCubeMapIndexing_(i); });
                        }
                        ifm.AddImageFilter(m_graph, kOmniShadowMapAdvBlurXPassNames[i], blurX);

                        ImageFilterDesc blurY      = {};
                        blurY.filter               = eImageFilter::Convolution1DY;
                        blurY.output               = eImageFilterOutput::Inject;
                        blurY.frameBuffer          = kOmniShadowMapAdvFBNames[i];
                        blurY.input0Texture        = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                        blurY.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";
                        ifm.AddImageFilter(m_graph, kOmniShadowMapAdvBlurYPassNames[i], blurY);

                        shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                    }
                }
            }
        }

        // lighting
        m_graph.AddRenderPass("Pass_OmniShadowReceiver")
            .SetFrameBuffer("FB_Light")
            .SetProgram("P_OmniShadowReceiver")
            .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
            .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
            .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
            .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
            .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadow::kSlot)
            .SetTexture("T_SceneNormal", eShader::PixelShader, kTSceneNormalSlot)
            .SetTexture("T_SceneColor", eShader::PixelShader, kTSceneColorSlot)
            .SetTexture("T_SceneDepth", eShader::PixelShader, kTSceneDepthSlot)
            .SetTexture("T_OmniShadowMap", eShader::PixelShader, kTOmniShadowMapSlot)
            .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
            .SetSampler(kShadowComparison, eShader::PixelShader, kSShadowComparisonSlot)
            .SetRenderFlags(kFrontCull)
            .SetBlendFlags(kAdditiveBlend, 0)
            .SetExecutor(Bind<&ShadowMappingScene::RenderOmniLightVolumes_>(this));

        if (bAdvFiltering)
        {
            m_graph.ModifyRenderPass("Pass_OmniShadowReceiver")
                .SetTexture("T_OmniShadowMapAdv", eShader::PixelShader, kTOmniShadowMapAdvSlot);
        }
    }

    // light + color 합성
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc multDesc = {};
    multDesc.filter          = eImageFilter::Mult;
    multDesc.input0Texture   = "T_SceneColor";
    multDesc.input1Texture   = "T_SceneLight";
    multDesc.output          = eImageFilterOutput::Inject;
    multDesc.frameBuffer     = "FB_SceneFinal";
    ifm.AddImageFilter(m_graph, "Pass_Mult", multDesc);

    // debug light cube
    if (m_activeLight == eLight::Spot || m_activeLight == eLight::Omni)
    {
        m_graph.AddRenderPass("Pass_DebugSpotLight")
            .SetFrameBuffer("FB_SceneFinal")
            .SetProgram("P_DebugLightCube")
            .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
            .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
            .SetRenderFlags(kBackCullDepthTest)
            .SetBlendFlags(kOpaqueBlend, 0)
            .SetExecutor(Bind<&ShadowMappingScene::RenderDebugLightCube_>(this));
    }

    // blit
    ImageFilterDesc blitDesc = {};
    blitDesc.filter          = eImageFilter::Blit;
    blitDesc.input0Texture   = "T_SceneFinal";
    blitDesc.output          = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);
}

void ShadowMappingScene::RenderScene_()
{
    Graphics& gfx = GetGraphics();

    CBEntity cb = {};
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

    CBShadowCaster caster;
    const Vec3     dir = m_spotLight.dir.Normalized();
    caster.lightView   = Mtx4::MakeInvSRT(Vec3::kOne, dir, Vec3::kUp, m_spotLight.pos).Transpose();
    caster.lightProj   = Mtx4::MakePerspectiveFov(m_spotLight.outerConeRad * 2.f, 1.f, 0.1f, m_spotLight.fallOffEnd).Transpose();
    gfx.UpdateBuffer(m_shadowCasterCbh, AsMemoryView(caster));

    CBEntity entity;
    for (const Entity& e: m_entities)
    {
        entity.world = e.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit(true);
    }
}

void ShadowMappingScene::RenderSceneOmniShadowCast_(
    const eCubeFace _omitFace)
{
    Graphics& gfx = GetGraphics();

    CBShadowCaster caster;
    switch (_omitFace)
    {
        case eCubeFace::PosX:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, Vec3::kRight, Vec3::kUp, m_omniLight.pos).Transpose();
            break;
        case eCubeFace::NegX:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, -Vec3::kRight, Vec3::kUp, m_omniLight.pos).Transpose();
            break;
        case eCubeFace::PosY:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, Vec3::kUp, -Vec3::kForward, m_omniLight.pos).Transpose();
            break;
        case eCubeFace::NegY:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, -Vec3::kUp, Vec3::kForward, m_omniLight.pos).Transpose();
            break;
        case eCubeFace::PosZ:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, Vec3::kForward, Vec3::kUp, m_omniLight.pos).Transpose();
            break;
        case eCubeFace::NegZ:
            caster.lightView = Mtx4::MakeInvSRT(Vec3::kOne, -Vec3::kForward, Vec3::kUp, m_omniLight.pos).Transpose();
            break;
        default:
            JAM_UNREACHABLE;
    }
    caster.lightProj = Mtx4::MakePerspectiveFov(m_spotLight.outerConeRad * 2.f, 1.f, 0.1f, m_spotLight.fallOffEnd).Transpose();
    gfx.UpdateBuffer(m_shadowCasterCbh, AsMemoryView(caster));

    CBEntity entity;
    for (const Entity& e: m_entities)
    {
        entity.world = e.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit(true);
    }
}

void ShadowMappingScene::RenderScreenTriWithCubeMapIndexing_(
    const Int32 _index) const
{
    Graphics&           gfx = GetGraphics();
    ImageFilterManager& ifm = GetImageFilterManager();

    CBImageFilterParam param;
    param.iparam.x = _index;
    gfx.UpdateBuffer(m_cubeMapIndexingCbh, AsMemoryView(param));
    ifm.SubmitScreenTriangle();
}

void ShadowMappingScene::RenderSpotLightVolumes_() const
{
    Graphics&   gfx = GetGraphics();
    const float h   = m_spotLight.fallOffEnd;
    const float r   = h * Tan(m_spotLight.outerConeRad);
    const Vec3  dir = m_spotLight.dir.Normalized();

    CBEntity entity;
    entity.world = Mtx4::MakeSRT({ r, r, h }, dir, Vec3::kUp, m_spotLight.pos).Transpose();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

    CBLight light;
    light.lightPos          = m_spotLight.pos;
    light.lightDir          = dir;
    light.lightFallOffStart = m_spotLight.fallOffStart;
    light.lightFallOffEnd   = m_spotLight.fallOffEnd;
    light.lightSpotInnerCos = Cos(m_spotLight.innerConeRad);
    light.lightSpotOuterCos = Cos(m_spotLight.outerConeRad);
    light.bMakeShadow       = m_spotLight.bCastShadow;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_pCone->Submit();
}

void ShadowMappingScene::RenderOmniLightVolumes_() const
{
    Graphics& gfx = GetGraphics();

    CBEntity entity;
    entity.world = Mtx4::MakeSRT(Vec3 { m_omniLight.fallOffEnd }, Vec3::kZero, m_omniLight.pos).Transpose();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

    CBLight light;
    light.lightPos          = m_omniLight.pos;
    light.lightFallOffStart = m_omniLight.fallOffStart;
    light.lightFallOffEnd   = m_omniLight.fallOffEnd;
    light.bMakeShadow       = m_omniLight.bCastShadow;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_pSphere->Submit();
}

void ShadowMappingScene::RenderDirectionalLightVolumes_() const
{
    ImageFilterManager& ifm = GetImageFilterManager();
    ifm.SubmitScreenTriangle();
}

void ShadowMappingScene::RenderDebugLightCube_() const
{
    CBEntity entity;
    if (m_activeLight == eLight::Spot)
    {
        entity.world = Mtx4::MakeSRT(Vec3::kOne, m_spotLight.dir, Vec3::kUp, m_spotLight.pos).Transpose();
    }
    else if (m_activeLight == eLight::Omni)
    {
        entity.world = Mtx4::MakeSRT(Vec3::kOne, Quat::kIdentity, m_omniLight.pos).Transpose();
    }

    CBCamera camera;
    camera.viewProj = m_camera.CalcViewProj().Transpose();

    Graphics& gfx = GetGraphics();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));
    m_pCube->Submit();
}

void ShadowMappingScene::PrepareShadowPass_() const
{
    CBShadow shadow;
    shadow.shadowType            = AsUnderlying(m_shadowFilter);
    shadow.bUseComparisonSampler = m_bUseComparisonSampler;
    shadow.shadowBias            = m_shadowBias;
    shadow.shadowNormalOffset    = m_shadowNormalOffset;
    shadow.esmExponential        = m_esmExponent;

    Graphics& gfx = GetGraphics();
    gfx.UpdateBuffer(m_shadowCbh, AsMemoryView(shadow));
}

Int32 ShadowMappingScene::GetShadowMapLength_() const
{
    switch (m_shadowMapSize)
    {
        case eShadowMapSize::x256:
            return 256;
        case eShadowMapSize::x512:
            return 512;
        case eShadowMapSize::x1024:
            return 1024;
        case eShadowMapSize::x2048:
            return 2048;
        case eShadowMapSize::x4096:
            return 4096;
        default:
            JAM_UNREACHABLE;
    }
}