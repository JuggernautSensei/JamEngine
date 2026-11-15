#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

class DeferredScene : public Scene
{
    JAM_SCENE_BODY(DeferredScene);

    static constexpr Int32 kMaxLights = 2048;

public:
    DeferredScene();
    ~DeferredScene() noexcept override;

    DeferredScene(const DeferredScene&)            = delete;
    DeferredScene(DeferredScene&&)                 = delete;
    DeferredScene& operator=(const DeferredScene&) = delete;
    DeferredScene& operator=(DeferredScene&&)      = delete;

    void OnUpdate(float _deltaSec) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;
    void InitMeshes_();
    void InitRenderGraph_();

    void SubmitGBuffer_() const;
    void SubmitLights_();

    float m_dLightMove = 0.5f;
    float m_dRot       = ToRad(45.f);
    float m_counter    = 0.f;
    float m_rot        = 0.f;
    bool  m_bAnimate   = true;

    RenderGraph m_graph = {};

    Ref<Mesh>    m_pDragon       = nullptr;
    Ref<Texture> m_pDragonAlbedo = nullptr;
    Ref<Texture> m_pDragonNormal = nullptr;

    Ref<Mesh>    m_pBunny       = nullptr;
    Ref<Texture> m_pBunnyAlbedo = nullptr;
    Ref<Texture> m_pBunnyNormal = nullptr;

    Ref<Mesh> m_pPlane = nullptr;

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    Int32 m_numLights          = 512;
    bool  m_bUseScissor        = true;
    bool  m_bVisualizeScissor  = false;
    bool  m_bShowColorTextures = true;
    Int32 m_previewLight       = 0;
    struct ScissorPreview
    {
        Rect  rect;
        Vec3  color;
        Int32 lightIndex;
    };
    Vector<ScissorPreview> m_scissorPreviews;

    FlyingCamera m_camera = {};
};
