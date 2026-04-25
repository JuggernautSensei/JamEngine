#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/DebugRenderer.h"

class DeferedScene : public Scene
{
    JAM_SCENE_BODY(DeferedScene);

    struct Light
    {
        Vec3  pos;
        Vec3  offset;
        Vec3  color;
        float radius;
        float freqX, freqY, freqZ;
        float phaseX, phaseY, phaseZ;
        float ampX, ampY, ampZ;
    };
    static constexpr Int32 kMaxLights = 2048;

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
    void InitLights_(Int32 _from = 0, Int32 _to = kMaxLights);

    float m_dLightMove = 0.5f;
    float m_dRot       = ToRad(45.f);
    float m_counter    = 0.f;
    float m_rot        = 0.f;
    bool  m_bAnimate   = true;

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

    Array<Light, kMaxLights> m_lights    = {};
    Int32                    m_numLights = 100;

    FlyingCamera m_camera = {};
};
