#include "pch.h"
#include "ShadowMappingScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RandomGenerator.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/Filter.h"
#include "../JamEngine/Random.h"

#define SCENE_PATH(_filepath) "Resources/ShadowMappingScene/" _filepath

namespace
{

constexpr Int32 kTSceneNormalSlot = 0;
constexpr Int32 kTSceneColorSlot  = 1;
constexpr Int32 kTSceneDepthSlot  = 2;
constexpr Int32 kTSceneLightSlot  = 3;
constexpr Int32 kTAlbedoSlot      = 4;

constexpr Int32 kTShadowMapSlot           = 5;
constexpr Int32 kTShadowMapColorSlot      = 6;
constexpr Int32 kTShadowMapArraySlot      = 7;
constexpr Int32 kTShadowMapColorArraySlot = 8;

constexpr Int32 kTImageFilterInput0 = 0;

constexpr Int32 kSLinearClampSlot      = 0;
constexpr Int32 kSLinearWrapSlot       = 1;
constexpr Int32 kSShadowComparisonSlot = 2;

}   // namespace

ShadowMappingScene::ShadowMappingScene()
{
    // spot
    {
        ImageFilterManager& ifm = GetImageFilterManager();

        ShadowSpec& spec                  = m_shadowSpecs[eLight::Spot];
        spec.depthBuffer                  = "T_SpotShadowMapDepth";
        spec.colorBuffer                  = "T_SpotShadowMapColor";
        spec.casterProgram                = "P_ShadowCaster";
        spec.vsmCasterProgram             = "P_ShadowCasterVSM";
        spec.receiverProgram              = "P_SpotShadowReceiver";
        spec.frameBuffers[0]              = "FB_SpotShadowMap";
        spec.casterPasses[0]              = "Pass_SpotShadowCaster";
        spec.advFrameBuffers[0]           = "FB_SpotShadowMapAdv";
        spec.advBlurXPasses[0]            = "Pass_SpotShadowMapAdvBlurX";
        spec.advBlurYPasses[0]            = "Pass_SpotShadowMapAdvBlurY";
        spec.advAAPasses[0]               = "Pass_SpotShadowMapAdvAA";
        spec.casterExecutor[0]            = Bind<&ShadowMappingScene::RenderSpotShadowCasters_>(this);
        spec.screenTriIndexingExecutor[0] = Bind<&ImageFilterManager::SubmitScreenTriangle>(&ifm);
        spec.lightVolumeExecutor          = Bind<&ShadowMappingScene::RenderSpotLightVolume_>(this);
    }

    // omni
    {
        constexpr StringView kPrefixes[] = { "PosX", "NegX", "PosY", "NegY", "PosZ", "NegZ" };

        ShadowSpec& spec      = m_shadowSpecs[eLight::Omni];
        spec.depthBuffer      = "T_OmniShadowMapDepth";
        spec.colorBuffer      = "T_OmniShadowMapColor";
        spec.casterProgram    = "P_OmniShadowCaster";
        spec.vsmCasterProgram = "P_ShadowCasterVSM";
        spec.receiverProgram  = "P_OmniShadowReceiver";
        spec.bIsArray         = true;

        for (Int32 i = 0; i < 6; ++i)
        {
            spec.frameBuffers[i]              = String::Formatted("FB_OmniShadowMap_{}", kPrefixes[i]);
            spec.casterPasses[i]              = String::Formatted("Pass_OmniShadowCaster_{}", kPrefixes[i]);
            spec.advFrameBuffers[i]           = String::Formatted("FB_OmniShadowMapAdv_{}", kPrefixes[i]);
            spec.advBlurXPasses[i]            = String::Formatted("Pass_OmniShadowMapAdvBlurX_{}", kPrefixes[i]);
            spec.advBlurYPasses[i]            = String::Formatted("Pass_OmniShadowMapAdvBlurY_{}", kPrefixes[i]);
            spec.advAAPasses[i]               = String::Formatted("Pass_OmniShadowMapAdvAA_{}", kPrefixes[i]);
            spec.casterExecutor[i]            = Bind([i, this] { RenderOmniShadowCasters_(RangesOf<eCubeFace>()[i]); });
            spec.screenTriIndexingExecutor[i] = Bind([i, this] { RenderScreenTriIndexing_(i); });
            spec.lightVolumeExecutor          = Bind<&ShadowMappingScene::RenderOmniLightVolume_>(this);
        }
    }

    // dir
    {
        constexpr StringView kPrefixes[] = { "0", "1", "2", "3" };

        ShadowSpec& spec      = m_shadowSpecs[eLight::Directional];
        spec.depthBuffer      = "T_CascadeShadowMapDepth";
        spec.colorBuffer      = "T_CascadeShadowMapColor";
        spec.casterProgram    = "P_CascadeShadowCaster";
        spec.vsmCasterProgram = "P_CascadeShadowCasterVSM";
        spec.receiverProgram  = "P_CascadeShadowReceiver";
        spec.bIsArray         = true;

        for (Int32 i = 0; i < 4; ++i)
        {
            spec.frameBuffers[i]              = String::Formatted("FB_CascadeShadowMap_{}", kPrefixes[i]);
            spec.casterPasses[i]              = String::Formatted("Pass_CascadeShadowCaster_{}", kPrefixes[i]);
            spec.advFrameBuffers[i]           = String::Formatted("FB_CascadeShadowMapAdv_{}", kPrefixes[i]);
            spec.advBlurXPasses[i]            = String::Formatted("Pass_CascadeShadowMapAdvBlurX_{}", kPrefixes[i]);
            spec.advBlurYPasses[i]            = String::Formatted("Pass_CascadeShadowMapAdvBlurY_{}", kPrefixes[i]);
            spec.advAAPasses[i]               = String::Formatted("Pass_CascadeShadowMapAdvAA_{}", kPrefixes[i]);
            spec.casterExecutor[i]            = Bind([i, this] { RenderCascadeShadowCasters_(i); });
            spec.screenTriIndexingExecutor[i] = Bind([i, this] { RenderScreenTriIndexing_(i); });
            spec.lightVolumeExecutor          = Bind<&ShadowMappingScene::RenderDirectionalLightVolume_>(this);
        }
    }

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

    m_dirLight.dir = Vec3 { -1.f, -1.f, 1.f }.Normalized();

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

    // prepare camera
    const Mtx4 viewProj = m_camera.CalcViewProj();

    CBCamera camera;
    camera.viewProj    = viewProj.Transpose();
    camera.invViewProj = m_camera.CalcInvViewProj().Transpose();
    camera.eyePos      = m_camera.GetEyePos();
    camera.eyeDir      = m_camera.GetForward();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    if (m_activeLight == eLight::Directional)
    {
        // calc distances
        Frustum frustumInV { m_camera.GetProj() };
        frustumInV.Normalize();

        Array<CascadeSplit, 4> dists = {};
        frustumInV.CalcCascadeSplitsInViewSpace(Span { dists.data(), m_cascadeLevel }, m_cascadeSplitWeight);

        // calc matrix
        for (Int32 i = 0; i < m_cascadeLevel; ++i)
        {
            const auto [view, proj]                         = CalcCascadeLightViewProj_(dists[i].nearZ, dists[i].farZ);
            m_directionalShadowReceiver.cascadeDists[i]     = dists[i].farZ;
            m_directionalShadowReceiver.cascadeLightView[i] = view.Transpose();
            m_directionalShadowReceiver.cascadeLightProj[i] = proj.Transpose();
        }
        gfx.UpdateBuffer(m_csmCasterCbh, AsMemoryView(m_directionalShadowReceiver));
    }

    if (m_activeLight != eLight::None)
    {
        CBShadow shadow;
        shadow.shadowType            = AsUnderlying(m_shadowMode);
        shadow.bUseComparisonSampler = m_bUseComparisonSampler;
        shadow.bRenderLightRegion    = m_bRenderLightRegion;
        shadow.shadowBias            = m_shadowBias;
        shadow.shadowNormalOffset    = m_shadowNormalOffset;
        shadow.cascadeLevel          = m_cascadeLevel;
        gfx.UpdateBuffer(m_shadowCbh, AsMemoryView(shadow));
    }

    // clear light fb. (you can move this code at utility pass)
    {
        const FrameBufferHandle fbh = m_graph.GetFrameBuffer("FB_Light");
        gfx.ClearRenderTarget(fbh, RGBA::FromLinear(0.1f, 0.1f, 0.1f));
    }

    m_graph.Execute(_view.fbh, _view.viewport);
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

    const bool bIsHard = m_shadowMode == eShadowMode::Hard;
    const bool bIsPCF  = m_shadowMode == eShadowMode::PCF3 || m_shadowMode == eShadowMode::PCF5 || m_shadowMode == eShadowMode::PCF7;

    ////////// Animation //////////

    ImGui::Checkbox("animate", &m_bAnimate);
    ImGui::SameLine();
    ImGui::SliderFloat("rot speed", &m_dRot, 0.f, ToRad(360.f), "%.2f rad/s");

    ImGui::Spacing();

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

    ImGui::Spacing();

    ////////// Light Properties //////////

    if (m_activeLight == eLight::Spot)
    {
        if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            bNeedRebuildGraph |= ImGui::Checkbox("render light region##spot", &m_bRenderLightRegion);

            ImGui::SeparatorText("Transform");
            ImGui::DragFloat3("position##spot", &m_spotLight.pos.x, 0.1f);
            ImGui::DragFloat3("direction##spot", &m_spotLight.dir.x, 0.01f, -1.f, 1.f);

            ImGui::SeparatorText("Range");
            ImGui::DragFloat("fall off start##spot", &m_spotLight.fallOffStart, 0.1f, 0.f, m_spotLight.fallOffEnd);
            ImGui::DragFloat("fall off end##spot", &m_spotLight.fallOffEnd, 0.1f, m_spotLight.fallOffStart, FLT_MAX);

            ImGui::SeparatorText("Cone");
            if (ImGui::SliderAngle("inner cone##spot", &m_spotLight.innerConeRad, 1.f, 89.f))
            {
                m_spotLight.innerConeRad = Min(m_spotLight.innerConeRad, m_spotLight.outerConeRad - ToRad(1.f));
            }
            if (ImGui::SliderAngle("outer cone##spot", &m_spotLight.outerConeRad, 1.f, 89.f))
            {
                m_spotLight.outerConeRad = Max(m_spotLight.outerConeRad, m_spotLight.innerConeRad + ToRad(1.f));
            }

            ImGui::Unindent();
        }
    }
    else if (m_activeLight == eLight::Omni)
    {
        if (ImGui::CollapsingHeader("Omni Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            bNeedRebuildGraph |= ImGui::Checkbox("render light region##omni", &m_bRenderLightRegion);

            ImGui::SeparatorText("Transform");
            ImGui::DragFloat3("position##omni", &m_omniLight.pos.x, 0.1f);

            ImGui::SeparatorText("Range");
            ImGui::DragFloat("fall off start##omni", &m_omniLight.fallOffStart, 0.1f, 0.f, m_omniLight.fallOffEnd);
            ImGui::DragFloat("fall off end##omni", &m_omniLight.fallOffEnd, 0.1f, m_omniLight.fallOffStart, FLT_MAX);

            ImGui::Unindent();
        }
    }
    else if (m_activeLight == eLight::Directional)
    {
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            bNeedRebuildGraph |= ImGui::Checkbox("render light region##dir", &m_bRenderLightRegion);

            ImGui::SeparatorText("Transform");
            ImGui::DragFloat3("direction##dir", &m_dirLight.dir.x, 0.01f, -1.f, 1.f);

            ImGui::SeparatorText("Cascade");
            bNeedRecreateShadowTextures |= ImGui::DragInt("cascade level", &m_cascadeLevel, 0.1f, 1, 4);
            ImGui::DragFloat("cascade z offset", &m_cascadeZOffset, 0.1f, 0.f, 100.f);
            ImGui::DragFloat("cascade split weight", &m_cascadeSplitWeight, 0.01f, 0.f, 1.f);

            ImGui::Unindent();
        }
    }

    ImGui::Spacing();

    ////////// Shadow //////////

    if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        // Resolution
        ImGui::SeparatorText("Resolution");
        {
            constexpr const char* kResolutions[] = {
                "256 x 256", "512 x 512", "1024 x 1024", "2048 x 2048", "4096 x 4096"
            };
            const char* display = kResolutions[static_cast<int>(m_shadowMapSize)];

            if (ImGui::SliderInt("##resolution", reinterpret_cast<int*>(&m_shadowMapSize), static_cast<int>(eShadowMapSize::x256), static_cast<int>(eShadowMapSize::x4096), display))
            {
                bNeedRecreateShadowTextures = true;
            }
        }

        // Filtering
        ImGui::SeparatorText("Filtering");
        {
            int filter = static_cast<int>(m_shadowMode);

            bNeedRebuildGraph |= ImGui::RadioButton("Hard", &filter, static_cast<int>(eShadowMode::Hard));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF 3", &filter, static_cast<int>(eShadowMode::PCF3));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF 5", &filter, static_cast<int>(eShadowMode::PCF5));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("PCF 7", &filter, static_cast<int>(eShadowMode::PCF7));
            ImGui::SameLine();
            bNeedRebuildGraph |= ImGui::RadioButton("VSM", &filter, static_cast<int>(eShadowMode::VSM));

            m_shadowMode = static_cast<eShadowMode>(filter);
        }

        ImGui::Spacing();

        // comparison sampler: PCF 계열에서만 유효
        ImGui::BeginDisabled(!bIsPCF);
        ImGui::Checkbox("comparison sampler", &m_bUseComparisonSampler);
        ImGui::EndDisabled();

        // blur / AA: VSM 전용
        ImGui::BeginDisabled(m_shadowMode != eShadowMode::VSM);
        bNeedRebuildGraph |= ImGui::Checkbox("apply blur", &m_bApplyBlur);
        ImGui::SameLine();
        bNeedRebuildGraph |= ImGui::Checkbox("apply anti-aliasing", &m_bApplyAA);
        ImGui::EndDisabled();

        // Parameters
        ImGui::SeparatorText("Parameters");

        // bias, normal offset
        ImGui::BeginDisabled(m_shadowMode == eShadowMode::VSM);
        ImGui::DragFloat("bias##shadow", &m_shadowBias, 0.0001f, 0.f, 0.1f, "%.4f");
        ImGui::DragFloat("normal offset##shadow", &m_shadowNormalOffset, 0.0001f, 0.f, 1.f, "%.4f");
        ImGui::EndDisabled();

        ImGui::Unindent();
    }

    ////////// 그래프 재빌드 / 텍스처 재생성 //////////

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
    m_camera.SetAspectRatioFromBackBuffer();
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
    m_graph.EmplaceConstantBuffer("CB_Indexing", sizeof(CBImageFilterParam));
    m_graph.EmplaceConstantBuffer("CB_CascadeShadowReceiver", sizeof(CBCascadeShadowReceiver));

    m_entityCbh       = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh          = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh        = m_graph.GetConstantBuffer("CB_Light");
    m_shadowCbh       = m_graph.GetConstantBuffer("CB_Shadow");
    m_shadowCasterCbh = m_graph.GetConstantBuffer("CB_ShadowCaster");
    m_indexingCbh     = m_graph.GetConstantBuffer("CB_Indexing");
    m_csmCasterCbh    = m_graph.GetConstantBuffer("CB_CascadeShadowReceiver");

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

    // gbuffer pass
    LoadProgram(m_graph, "P_GBuffer", SCENE_PATH("GBufferShader.hlsl"));

    // spot light
    {
        const ShadowSpec& spec = m_shadowSpecs[eLight::Spot];
        LoadProgram(m_graph, spec.casterProgram, SCENE_PATH("ShadowCasterShader.hlsl"));
        LoadProgram(m_graph, spec.receiverProgram, SCENE_PATH("SpotShadowReceiverShader.hlsl"));
    }

    // omni light
    {
        const ShadowSpec& spec = m_shadowSpecs[eLight::Omni];

        {
            ShaderMacro macro[1] = {};
            macro[0].name        = "LIGHT_OMNI";
            LoadProgram(m_graph, spec.casterProgram, SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
        }

        LoadProgram(m_graph, spec.receiverProgram, SCENE_PATH("OmniShadowReceiverShader.hlsl"));
    }

    // dir light
    {
        const ShadowSpec& spec = m_shadowSpecs[eLight::Directional];

        {
            ShaderMacro macro[1] = {};
            macro[0].name        = "LIGHT_DIRECTIONAL";
            LoadProgram(m_graph, spec.casterProgram, SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
        }

        LoadProgram(m_graph, spec.receiverProgram, SCENE_PATH("CascadeShadowReceiverShader.hlsl"));
    }

    // adv mode
    {
        {
            ShaderMacro macro[1] = {};
            macro[0].name        = "SHADOW_MODE_VSM";
            LoadProgram(m_graph, "P_ShadowCasterVSM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
        }

        {
            ShaderMacro macro[2] = {};
            macro[0].name        = "LIGHT_DIRECTIONAL";
            macro[1].name        = "SHADOW_MODE_VSM";
            LoadProgram(m_graph, "P_CascadeShadowCasterVSM", SCENE_PATH("ShadowCasterShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
        }
    }

    // misc
    LoadProgram(m_graph, "P_DebugLightCube", SCENE_PATH("DebugLightCubeShader.hlsl"));
}

void ShadowMappingScene::InitRenderGraphShadowTextures_()
{
    const Int32 shadowMapLength = GetShadowMapLength_();

    for (const eLight light: { eLight::Spot, eLight::Omni, eLight::Directional })
    {
        const ShadowSpec& spec      = m_shadowSpecs[light];
        const Int32       numLayers = light == eLight::Spot ? 1
                                    : light == eLight::Omni ? 6
                                                            : m_cascadeLevel;

        // shadow map
        m_graph.EmplaceTexture2D(spec.depthBuffer, shadowMapLength, shadowMapLength, eTextureFormat::D32_Float, false, numLayers, eTextureFlags::DepthStencil);
        m_graph.EmplaceTexture2D(spec.colorBuffer, shadowMapLength, shadowMapLength, eTextureFormat::RG32_Float, false, numLayers, eTextureFlags::RenderTarget);

        // shadow map frame buffer
        for (Int32 i = 0; i < numLayers; ++i)
        {
            AttachmentRG attachments[1] = {};
            attachments[0].name         = spec.depthBuffer;
            attachments[0].layerOffset  = i;
            m_graph.EmplaceFrameBuffer(spec.frameBuffers[i], attachments);
        }

        // advanced filtering frame buffer
        for (Int32 i = 0; i < numLayers; ++i)
        {
            AttachmentRG attachments[2] = {};
            attachments[0].name         = spec.colorBuffer;
            attachments[0].layerOffset  = i;
            attachments[1].name         = spec.depthBuffer;
            attachments[1].layerOffset  = i;
            m_graph.EmplaceFrameBuffer(spec.advFrameBuffers[i], attachments);
        }
    }
}

void ShadowMappingScene::InitRenderGraphPasses_()
{
    ImageFilterManager& ifm = GetImageFilterManager();

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

    // shadow pass
    if (m_activeLight != eLight::None)
    {
        const ShadowSpec& spec          = m_shadowSpecs[m_activeLight];
        const StringView  casterProgram = m_shadowMode == eShadowMode::VSM ? spec.vsmCasterProgram
                                                                           : spec.casterProgram;
        const Int32       numLayers     = m_activeLight == eLight::Spot ? 1
                                        : m_activeLight == eLight::Omni ? 6
                                                                        : m_cascadeLevel;
        StringView        shadowMap     = {};

        // shadow caster
        for (Int32 i = 0; i < numLayers; ++i)
        {
            RenderGraph::RenderPassBuilder builder =
                m_graph.AddRenderPass(spec.casterPasses[i])
                    .SetProgram(casterProgram)
                    .SetDepthClearCommand(1.f)
                    .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
                    .SetConstantBuffer("CB_ShadowCaster", eShader::VertexShader, CBShadowCaster::kSlot)
                    .SetConstantBuffer("CB_Shadow", eShader::PixelShader, CBShadow::kSlot)
                    .SetRenderFlags(kBackCullDepthTest)
                    .SetBlendFlags(kOpaqueBlend, 0)
                    .SetExecutor(spec.casterExecutor[i]);

            if (m_shadowMode == eShadowMode::VSM)
            {
                builder.SetFrameBuffer(spec.advFrameBuffers[i])
                    .SetRenderTargetClearCommand(RGBA::kWhite, 0);
                shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
            }
            else
            {
                builder.SetFrameBuffer(spec.frameBuffers[i]);
                shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), kDepthStencilSlot);
            }

            if (m_shadowMode == eShadowMode::VSM)
            {
                // anti aliasing
                if (m_bApplyAA)
                {
                    ImageFilterInfo aaInfo = {};
                    aaInfo.filter          = eImageFilter::FXAA;
                    aaInfo.output          = eImageFilterOutput::Create;
                    aaInfo.format          = eTextureFormat::RG32_Float;
                    aaInfo.width           = GetShadowMapLength_();
                    aaInfo.height          = GetShadowMapLength_();
                    aaInfo.input0Texture   = shadowMap;

                    if (spec.bIsArray)
                    {
                        aaInfo.bUseTexture2DArrayInput0 = true;
                        aaInfo.paramCBufferOrEmpty      = "CB_Indexing";
                        aaInfo.executorOrNull           = spec.screenTriIndexingExecutor[i];
                    }

                    ifm.AddImageFilter(m_graph, spec.advAAPasses[i], aaInfo);
                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                }

                // blur
                if (m_bApplyBlur)
                {
                    ImageFilterInfo blurX      = {};
                    blurX.filter               = eImageFilter::Convolution1DX;
                    blurX.output               = eImageFilterOutput::Create;
                    blurX.format               = eTextureFormat::RG32_Float;
                    blurX.width                = GetShadowMapLength_();
                    blurX.height               = GetShadowMapLength_();
                    blurX.input0Texture        = shadowMap;
                    blurX.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";

                    if (!m_bApplyAA)
                    {
                        blurX.bUseTexture2DArrayInput0 = true;
                        blurX.paramCBufferOrEmpty      = "CB_Indexing";
                        blurX.executorOrNull           = spec.screenTriIndexingExecutor[i];
                    }

                    ifm.AddImageFilter(m_graph, spec.advBlurXPasses[i], blurX);

                    ImageFilterInfo blurY      = {};
                    blurY.filter               = eImageFilter::Convolution1DY;
                    blurY.output               = eImageFilterOutput::Inject;
                    blurY.frameBuffer          = spec.advFrameBuffers[i];
                    blurY.input0Texture        = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                    blurY.kernelCBufferOrEmpty = "CB_GaussianFilterKernel";
                    ifm.AddImageFilter(m_graph, spec.advBlurYPasses[i], blurY);

                    shadowMap = m_graph.GetPassAttachment(m_graph.GetLastPass(), 0);
                }
            }
        }

        RenderGraph::RenderPassBuilder builder =
            m_graph.AddRenderPass("Pass_ShadowReceiver")
                .SetFrameBuffer("FB_Light")
                .SetProgram(spec.receiverProgram)
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
                .SetExecutor(spec.lightVolumeExecutor);

        if (m_shadowMode == eShadowMode::VSM)
        {
            builder.SetTexture(shadowMap, eShader::PixelShader, spec.bIsArray ? kTShadowMapColorArraySlot : kTShadowMapColorSlot);
        }
        else
        {
            builder.SetTexture(shadowMap, eShader::PixelShader, spec.bIsArray ? kTShadowMapArraySlot : kTShadowMapSlot);
        }

        if (m_activeLight == eLight::Directional)
        {
            builder.SetConstantBuffer("CB_CascadeShadowReceiver", eShader::PixelShader, CBCascadeShadowReceiver::kSlot)
                .SetRenderFlags(eRenderFlags::None);
        }
    }

    // light + color 합성
    {
        ImageFilterInfo multInfo = {};
        multInfo.filter          = eImageFilter::Mult;
        multInfo.input0Texture   = "T_SceneColor";
        multInfo.input1Texture   = "T_SceneLight";
        multInfo.output          = eImageFilterOutput::Inject;
        multInfo.frameBuffer     = "FB_SceneFinal";
        ifm.AddImageFilter(m_graph, "Pass_Mult", multInfo);
    }

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
    {
        ImageFilterInfo blitInfo = {};
        blitInfo.filter          = eImageFilter::Blit;
        blitInfo.input0Texture   = "T_SceneFinal";
        blitInfo.output          = eImageFilterOutput::Final;
        ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
    }
}

void ShadowMappingScene::RenderScene_()
{
    Graphics& gfx = GetGraphics();

    CBEntity entity = {};
    for (const Entity& e: m_entities)
    {
        entity.world       = e.transform.GetSRT().Transpose();
        entity.rgba        = e.rgba.rgba;
        entity.bUseTexture = !!e.pTextureOrNull;

        if (e.pTextureOrNull)
        {
            e.pTextureOrNull->Bind(eShader::PixelShader, kTAlbedoSlot);
        }

        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit();
    }
}

void ShadowMappingScene::RenderSpotShadowCasters_()
{
    Graphics& gfx = GetGraphics();

    CBShadowCaster caster;
    const Vec3     dir = m_spotLight.dir.Normalized();
    caster.lightView   = Mtx4::MakeInvSRT(Vec3::kOne, dir, Vec3::kUp, m_spotLight.pos).Transpose();
    caster.lightProj   = Mtx4::MakePerspective(m_spotLight.outerConeRad * 2.f, 1.f, 0.1f, m_spotLight.fallOffEnd).Transpose();
    gfx.UpdateBuffer(m_shadowCasterCbh, AsMemoryView(caster));

    CBEntity entity;
    for (const Entity& e: m_entities)
    {
        entity.world = e.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit(true);
    }
}

ShadowMappingScene::ViewProj ShadowMappingScene::CalcCascadeLightViewProj_(
    const float _nearZ,
    const float _farZ) const
{
    const Camera& cmr        = m_camera.camera;
    Corners       cornersInV = Frustum::CalcCornersInViewSpace(cmr.GetFovYRad(), cmr.GetAspectRatio(), _nearZ, _farZ);

    // world space corner
    const Mtx4& invV   = m_camera.GetInvView();
    Vec3        center = Vec3::kZero;
    for (Vec3& v: cornersInV)
    {
        v = invV.MultPoint(v);
        center += v;
    }

    // world center
    center /= 8.f;

    // light view
    const Mtx4 lightView = Mtx4::MakeLookTo(center, m_dirLight.dir, Vec3::kUp);

    // light corner
    Aabb lightAabb = {};
    for (Vec3& v: cornersInV)
    {
        v = lightView.MultPoint(v);
        lightAabb.Merge(v);
    }

    Vec3 min = lightAabb.GetMin();
    Vec3 max = lightAabb.GetMax();
    min.z -= m_cascadeZOffset;
    max.z += m_cascadeZOffset;

    const Mtx4 lightProj = Mtx4::MakeOrtho(min.x, max.x, min.y, max.y, min.z, max.z);
    return { lightView, lightProj };
}

void ShadowMappingScene::RenderOmniShadowCasters_(
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
    caster.lightProj = Mtx4::MakePerspective(ToRad(90.f), 1.f, 0.1f, m_omniLight.fallOffEnd).Transpose();
    gfx.UpdateBuffer(m_shadowCasterCbh, AsMemoryView(caster));

    CBEntity entity;
    for (const Entity& e: m_entities)
    {
        entity.world = e.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit(true);
    }
}

void ShadowMappingScene::RenderCascadeShadowCasters_(
    const Int32 _index)
{
    Graphics& gfx = GetGraphics();

    CBShadowCaster caster;
    caster.lightView = m_directionalShadowReceiver.cascadeLightView[_index];
    caster.lightProj = m_directionalShadowReceiver.cascadeLightProj[_index];
    gfx.UpdateBuffer(m_shadowCasterCbh, AsMemoryView(caster));

    CBEntity entity;
    for (const Entity& e: m_entities)
    {
        entity.world = e.transform.GetSRT().Transpose();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        e.pMesh->Submit(true);
    }
}

void ShadowMappingScene::RenderScreenTriIndexing_(
    const Int32 _index) const
{
    Graphics&           gfx = GetGraphics();
    ImageFilterManager& ifm = GetImageFilterManager();

    CBImageFilterParam param;
    param.iparam.x = _index;
    gfx.UpdateBuffer(m_indexingCbh, AsMemoryView(param));
    ifm.SubmitScreenTriangle();
}

void ShadowMappingScene::RenderSpotLightVolume_() const
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
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_pCone->Submit();
}

void ShadowMappingScene::RenderOmniLightVolume_() const
{
    Graphics& gfx = GetGraphics();

    CBEntity entity;
    entity.world = Mtx4::MakeSRT(Vec3 { m_omniLight.fallOffEnd }, Vec3::kZero, m_omniLight.pos).Transpose();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

    CBLight light;
    light.lightPos          = m_omniLight.pos;
    light.lightFallOffStart = m_omniLight.fallOffStart;
    light.lightFallOffEnd   = m_omniLight.fallOffEnd;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_pSphere->Submit();
}

void ShadowMappingScene::RenderDirectionalLightVolume_() const
{
    Graphics& gfx = GetGraphics();

    CBLight light;
    light.lightDir = m_dirLight.dir.Normalized();
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    ImageFilterManager& ifm = GetImageFilterManager();
    ifm.SubmitScreenTriangle();
}

void ShadowMappingScene::RenderDebugLightCube_() const
{
    CBEntity entity;
    if (m_activeLight == eLight::Spot)
    {
        const Vec3 f = m_spotLight.dir.Normalized();
        entity.world = Mtx4::MakeSRT(Vec3::kOne, f, Vec3::kUp, m_spotLight.pos).Transpose();
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
    shadow.shadowType            = AsUnderlying(m_shadowMode);
    shadow.bUseComparisonSampler = m_bUseComparisonSampler;
    shadow.shadowBias            = m_shadowBias;
    shadow.shadowNormalOffset    = m_shadowNormalOffset;

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