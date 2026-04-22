#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/DebugRenderer.h"
#include "../JamEngine/RenderGraph.h"

class ShadowMappingScene : public Scene
{
    JAM_SCENE_BODY(ShadowMappingScene);

    struct Entity
    {
        Ref<Mesh> pMesh     = nullptr;
        Transform transform = {};
    };

    enum class eLight : UInt32
    {
        Directional,
        Point,
        Spot,
    };

    struct Light
    {
        bool bEnabled    = false;
        bool bCastShadow = false;

        Vec3  pos          = Vec3::kZero;
        Vec3  lookAt       = Vec3::kZero;
        float fallOffStart = 0.f;
        float fallOffEnd   = 0.f;
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
    void InitMeshes_();
    void InitRenderGraph_();
    void RenderMeshes_();
    void RenderLightVolumes_() const;
    void RenderFullScreenTri_() const;

    void AddProgram_VertexPixel_(StringView _name, StringView _path);

    float m_time           = 0.f;
    float m_deltaAnimation = 1.f;
    float m_deltaLightMove = 0.5f;
    bool  m_bAnimate       = true;

    Vector<Entity> m_entities = {};
    Ref<Mesh>      m_pFsTri   = nullptr;
    Ref<Mesh>      m_pSphere  = nullptr;
    Ref<Mesh>      m_pCone    = nullptr;

    Light m_omniLight = {};
    Light m_spotLight = {};
    Light m_dirLight  = {};

    RenderGraph  m_graph  = {};
    FlyingCamera m_camera = {};

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;

    bool m_bUseShadow = false;
};
