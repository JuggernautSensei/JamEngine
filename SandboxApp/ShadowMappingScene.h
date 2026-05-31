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

    enum class eShadowFiltering
    {
        Hard,
        PCF3,
        PCF5,
        PCF7,
        VSM,
        ESM,
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
        bool bCastShadow = false;

        Vec3  pos          = Vec3::kZero;
        Vec3  dir          = { 0.f, 0.f, 1.f };
        float fallOffStart = 0.1f;
        float fallOffEnd   = 10.f;
        float innerConeRad = ToRad(30.f);
        float outerConeRad = ToRad(45.f);
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
    void RenderSceneSpotShadowCast_();
    void RenderSceneOmniShadowCast_(eCubeFace _omitFace);
    void RenderScreenTriWithCubeMapIndexing_(Int32 _index) const;

    // light volume
    void RenderSpotLightVolumes_() const;
    void RenderOmniLightVolumes_() const;
    void RenderDirectionalLightVolumes_() const;
    void RenderDebugLightCube_() const;

    // ect
    void                PrepareShadowPass_() const;
    [[nodiscard]] Int32 GetShadowMapLength_() const;

    float m_counter  = 0.f;
    float m_dRot     = ToRad(45.f);
    bool  m_bAnimate = true;

    constexpr static Int32        kTotalEntities = 20;
    Array<Entity, kTotalEntities> m_entities     = {};

    Ref<Mesh> m_pCone   = nullptr;
    Ref<Mesh> m_pSphere = nullptr;
    Ref<Mesh> m_pCube   = nullptr;

    Light  m_omniLight   = {};
    Light  m_spotLight   = {};
    Light  m_dirLight    = {};
    eLight m_activeLight = eLight::Spot;

    eShadowFiltering m_shadowFilter          = eShadowFiltering::Hard;
    float            m_shadowBias            = 0.0002f;
    float            m_shadowNormalOffset    = 0.01f;
    float            m_esmExponent           = 80.f;
    bool             m_bUseComparisonSampler = true;
    bool             m_bApplyBlur            = true;
    bool             m_bApplyAA              = true;

    eShadowMapSize m_shadowMapSize = eShadowMapSize::x1024;

    RenderGraph  m_graph   = {};
    FlyingCamera m_camera  = {};
    Frustum      m_frustum = {};

    ConstantBufferHandle m_entityCbh          = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh             = kInvalidHandle;
    ConstantBufferHandle m_lightCbh           = kInvalidHandle;
    ConstantBufferHandle m_shadowCbh          = kInvalidHandle;
    ConstantBufferHandle m_shadowCasterCbh    = kInvalidHandle;
    ConstantBufferHandle m_cubeMapIndexingCbh = kInvalidHandle;
};
