#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/DebugRenderer.h"

class DeferedScene : public Scene
{
    JAM_SCENE_BODY(DeferedScene);

    struct Light
    {
        Vec3  color  = Numbers<Vec3>::k_one;
        Vec3  pos    = Numbers<Vec3>::k_zero;
        Vec3  offset = Numbers<Vec3>::k_zero;
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

    ProgramHandle m_gbufferPh = k_nullHandle;
    ProgramHandle m_lightPh   = k_nullHandle;
    ProgramHandle m_combinePh = k_nullHandle;

    Ref<Mesh>    m_pDragon       = nullptr;
    Ref<Texture> m_pDragonAlbedo = nullptr;
    Ref<Texture> m_pDragonNormal = nullptr;

    Ref<Mesh>    m_pBunny       = nullptr;
    Ref<Texture> m_pBunnyAlbedo = nullptr;
    Ref<Texture> m_pBunnyNormal = nullptr;

    Ref<Mesh> m_pPlane = nullptr;

    VertexBufferHandle m_fullTriVbh = k_nullHandle;
    IndexBufferHandle  m_fullTriIbh = k_nullHandle;

    TextureHandle     m_colorTexh  = k_nullHandle;
    TextureHandle     m_normalTexh = k_nullHandle;
    TextureHandle     m_depthTexh  = k_nullHandle;
    FrameBufferHandle m_gbufferFbh = k_nullHandle;

    TextureHandle     m_lightTexh = k_nullHandle;
    FrameBufferHandle m_lightFbh  = k_nullHandle;
    Viewport          m_vp        = {};

    ConstantBufferHandle m_entityCbh = k_nullHandle;
    ConstantBufferHandle m_cameraCbh = k_nullHandle;
    ConstantBufferHandle m_lightCbh  = k_nullHandle;

    Array<Light, 1024> m_lights    = {};
    Int32              m_numLights = 100;

    FlyingCamera m_camera = {};
};
