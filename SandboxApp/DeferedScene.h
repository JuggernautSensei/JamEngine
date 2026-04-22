#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/DebugRenderer.h"

class DeferedScene : public Scene
{
    JAM_SCENE_BODY(DeferedScene);

    struct Light
    {
        Vec3  color  = Vec3::kOne;
        Vec3  pos    = Vec3::kZero;
        Vec3  offset = Vec3::kZero;
        float radius = 5.f;
    };

public:
    DeferedScene();
    ~DeferedScene() noexcept override;

    DeferedScene(const DeferedScene&)            = delete;
    DeferedScene(DeferedScene&&)                 = delete;
    DeferedScene& operator=(const DeferedScene&) = delete;
    DeferedScene& operator=(DeferedScene&&)      = delete;

    void OnUpdate(float _deltaSec) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;
    void InitLights();

    float m_time           = 0.f;
    float m_deltaAnimation = 1.f;
    float m_deltaLightMove = 0.5f;
    bool  m_bAnimate       = true;

    ProgramHandle m_gbufferPh = kInvalidHandle;
    ProgramHandle m_lightPh   = kInvalidHandle;
    ProgramHandle m_combinePh = kInvalidHandle;

    Ref<Mesh>    m_pDragon       = nullptr;
    Ref<Texture> m_pDragonAlbedo = nullptr;
    Ref<Texture> m_pDragonNormal = nullptr;

    Ref<Mesh>    m_pBunny       = nullptr;
    Ref<Texture> m_pBunnyAlbedo = nullptr;
    Ref<Texture> m_pBunnyNormal = nullptr;

    Ref<Mesh> m_pPlane = nullptr;

    VertexBufferHandle m_fullTriVbh = kInvalidHandle;
    IndexBufferHandle  m_fullTriIbh = kInvalidHandle;

    TextureHandle     m_colorTexh  = kInvalidHandle;
    TextureHandle     m_normalTexh = kInvalidHandle;
    TextureHandle     m_depthTexh  = kInvalidHandle;
    FrameBufferHandle m_gbufferFbh = kInvalidHandle;

    TextureHandle     m_lightTexh = kInvalidHandle;
    FrameBufferHandle m_lightFbh  = kInvalidHandle;
    Viewport          m_vp        = {};

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    Array<Light, 1024> m_lights    = {};
    Int32              m_numLights = 100;

    FlyingCamera m_camera = {};
};
