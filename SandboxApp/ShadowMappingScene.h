#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

enum class eImageFilter;

class ShadowMappingScene : public Scene
{
    JAM_SCENE_BODY(ShadowMappingScene);

    struct Entity
    {
        Ref<Mesh>    pMesh          = nullptr;
        Ref<Texture> pTextureOrNull = nullptr;
        Transform    transform      = {};
        RGBA         rgba           = RGBA::kWhite;
        bool         bNotAnimated   = false;
    };

    enum class eLight
    {
        None,
        Directional,
        Omni,
        Spot,
    };

    enum class eShadowMode
    {
        Hard,
        PCF3,
        PCF5,
        PCF7,
        VSM,
    };

    enum class eShadowMapSize
    {
        x256,
        x512,
        x1024,
        x2048,
        x4096,
    };

    struct Light
    {
        Vec3  pos          = Vec3::kZero;
        Vec3  dir          = { 0.f, 0.f, 1.f };
        float fallOffStart = 0.1f;
        float fallOffEnd   = 10.f;
        float innerConeRad = ToRad(30.f);
        float outerConeRad = ToRad(45.f);
    };

    struct ViewProj
    {
        Mtx4 view = Mtx4::kIdentity;
        Mtx4 proj = Mtx4::kIdentity;
    };

    struct ShadowSpec
    {
        String depthBuffer      = {};
        String colorBuffer      = {};
        String casterProgram    = {};
        String vsmCasterProgram = {};
        String receiverProgram  = {};

        Array<String, 6> frameBuffers    = {};
        Array<String, 6> advFrameBuffers = {};   // for adv filtering

        Array<String, 6> casterPasses   = {};
        Array<String, 6> advBlurXPasses = {};
        Array<String, 6> advBlurYPasses = {};
        Array<String, 6> advAAPasses    = {};

        Array<Delegate<void()>, 6> casterExecutor            = {};
        Array<Delegate<void()>, 6> screenTriIndexingExecutor = {};
        Delegate<void()>           lightVolumeExecutor       = {};

        bool bIsArray = false;
    };

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

        Vec3  lightPos;
        float lightFallOffStart;
        Vec3  lightDir;
        float lightFallOffEnd;
        float lightSpotInnerCos;
        float lightSpotOuterCos;
        float padding[2];
    };

    struct CBShadow
    {
        static constexpr Int32 kSlot = 3;

        UInt32 shadowType;
        UInt32 bUseComparisonSampler;
        UInt32 bRenderLightRegion;
        float  shadowBias;
        float  shadowNormalOffset;
        Int32  cascadeLevel;
        float  padding[2];
    };

    struct CBShadowCaster
    {
        static constexpr Int32 kSlot = 4;

        Mtx4 lightView;
        Mtx4 lightProj;
    };

    struct CBCascadeShadowReceiver
    {
        static constexpr Int32 kSlot = 5;

        Mtx4  cascadeLightView[4];
        Mtx4  cascadeLightProj[4];
        float cascadeDists[4];
    };

public:
    ShadowMappingScene();
    ~ShadowMappingScene() noexcept override;

    ShadowMappingScene(const ShadowMappingScene&)            = delete;
    ShadowMappingScene(ShadowMappingScene&&)                 = delete;
    ShadowMappingScene& operator=(const ShadowMappingScene&) = delete;
    ShadowMappingScene& operator=(ShadowMappingScene&&)      = delete;

    void OnUpdate(float _dt) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMeshes_();
    void InitRenderGraphResources_();
    void InitRenderGraphShadowTextures_();
    void InitRenderGraphPasses_();

    // render scene
    void RenderScene_();
    void RenderSpotShadowCasters_();
    void RenderOmniShadowCasters_(eCubeFace _omitFace);
    void RenderCascadeShadowCasters_(Int32 _index);
    void RenderScreenTriIndexing_(Int32 _index) const;

    // light volume
    void RenderSpotLightVolume_() const;
    void RenderOmniLightVolume_() const;
    void RenderDirectionalLightVolume_() const;
    void RenderDebugLightCube_() const;

    // ect
    [[nodiscard]] ViewProj CalcCascadeLightViewProj_(float _nearZ, float _farZ) const;

    void                PrepareShadowPass_() const;
    [[nodiscard]] Int32 GetShadowMapLength_() const;

    float m_counter  = 0.f;
    float m_dRot     = ToRad(45.f);
    bool  m_bAnimate = true;

    constexpr static Int32        kTotalEntities = 10;
    Array<Entity, kTotalEntities> m_entities     = {};

    Ref<Mesh> m_pCone   = nullptr;
    Ref<Mesh> m_pSphere = nullptr;
    Ref<Mesh> m_pCube   = nullptr;

    Light  m_spotLight   = {};
    Light  m_omniLight   = {};
    Light  m_dirLight    = {};
    eLight m_activeLight = eLight::Spot;

    eShadowMode m_shadowMode            = eShadowMode::Hard;
    float       m_shadowBias            = 0.0002f;
    float       m_shadowNormalOffset    = 0.01f;
    float       m_esmExponent           = 80.f;
    float       m_cascadeSplitWeight    = 0.8f;
    bool        m_bUseComparisonSampler = true;
    bool        m_bRenderLightRegion    = false;
    bool        m_bApplyBlur            = true;
    bool        m_bApplyAA              = true;

    // cascade
    CBCascadeShadowReceiver m_directionalShadowReceiver = {};
    Int32                   m_cascadeLevel              = 3;
    float                   m_cascadeZOffset            = 50.f;

    EnumArray<eLight, ShadowSpec> m_shadowSpecs   = {};
    eShadowMapSize                m_shadowMapSize = eShadowMapSize::x1024;

    RenderGraph  m_graph  = {};
    FlyingCamera m_camera = {};

    ConstantBufferHandle m_entityCbh       = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh          = kInvalidHandle;
    ConstantBufferHandle m_lightCbh        = kInvalidHandle;
    ConstantBufferHandle m_shadowCbh       = kInvalidHandle;
    ConstantBufferHandle m_shadowCasterCbh = kInvalidHandle;
    ConstantBufferHandle m_indexingCbh     = kInvalidHandle;
    ConstantBufferHandle m_csmCasterCbh    = kInvalidHandle;
};
