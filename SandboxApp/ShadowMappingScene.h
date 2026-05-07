#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/DebugRenderer.h"
#include "../JamEngine/RenderGraph.h"

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

    enum class eLight : UInt32
    {
        Directional,
        Point,
        Spot,
    };

    enum class eShadowSampling : UInt32
    {
        Hard,
        PCF3,
        PCF5,
        PCF7,
        VSM,
        ESM,
    };

    struct Light
    {
        bool bEnabled    = false;
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
    void InitRenderGraph_();
    void BuildRenderGraph_();

    // utils
    void AddProgram_VertexPixel_(StringView _name, StringView _path);
    void AddProgram_VertexPixelGeometry_(StringView _name, StringView _path);
    void AddProgram_VertexPixel_Macro_(StringView _name, StringView _path, StringView _macroDef, StringView _macroValue);

    // render scene
    void RenderScene_();
    void RenderSceneSpotShadowCast_();

    // light volume
    void RenderSpotLightVolumes_() const;
    void RenderPointLightVolumes_() const;
    void RenderDirectionalLightVolumes_() const;

    // ect
    void RenderFullScreenTri_() const;
    void PrepareShadowPass_() const;

    float m_counter  = 0.f;
    float m_dRot     = ToRad(45.f);
    bool  m_bAnimate = true;

    constexpr static Int32        kTotalEntities = 20;
    Array<Entity, kTotalEntities> m_entities     = {};

    Ref<Mesh> m_pFsTri  = nullptr;
    Ref<Mesh> m_pCone   = nullptr;
    Ref<Mesh> m_pSphere = nullptr;

    Light m_omniLight = {};
    Light m_spotLight = {};
    Light m_dirLight  = {};

    eShadowSampling m_shadowSampling        = eShadowSampling::Hard;
    float           m_shadowBias            = 0.0002f;
    float           m_shadowNormalOffset    = 0.01f;
    bool            m_bUseComparisonSampler = true;
    float           m_esmExponent           = 80.f;

    RenderGraph  m_graph  = {};
    FlyingCamera m_camera = {};

    ConstantBufferHandle m_entityCbh            = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh               = kInvalidHandle;
    ConstantBufferHandle m_lightCbh             = kInvalidHandle;
    ConstantBufferHandle m_shadowCbh            = kInvalidHandle;
    ConstantBufferHandle m_shadowCastCbh        = kInvalidHandle;
    ConstantBufferHandle m_omniShadowCastCbh    = kInvalidHandle;
    ConstantBufferHandle m_cascadeShadowCastCbh = kInvalidHandle;
};
