#pragma once
#include "FlyingCamera.h"

class SSAOScene : public Scene
{
    JAM_SCENE_BODY(SSAOScene);

    static constexpr Int32 kKernelSize = 64;

public:
    SSAOScene();
    ~SSAOScene() noexcept override;

    SSAOScene(const SSAOScene&)                = delete;
    SSAOScene& operator=(const SSAOScene&)     = delete;
    SSAOScene(SSAOScene&&) noexcept            = delete;
    SSAOScene& operator=(SSAOScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitModel_();
    void InitKernel_();
    void InitRenderGraph_();

    void DrawNode_(const ModelNode& _node, const Mtx4& _parentWorld) const;
    void SubmitGBuffer_() const;
    void SubmitSsao_() const;
    void SubmitScreenTriangle_() const;

    RenderGraph          m_graph        = {};
    ConstantBufferHandle m_entityCbh    = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh    = kInvalidHandle;
    ConstantBufferHandle m_ssaoCbh      = kInvalidHandle;
    ConstantBufferHandle m_compositeCbh = kInvalidHandle;

    // model
    Ref<Model> m_pModel = nullptr;

    // ssao
    Array<Vec4, kKernelSize> m_kernel    = {};
    TextureHandle            m_noiseTexh = kInvalidHandle;

    // camera
    FlyingCamera m_camera;

    // ui
    bool  m_bUseAo  = true;
    bool  m_bAoOnly = false;
    float m_radius  = 0.5f;
    float m_bias    = 0.025f;
    float m_power   = 2.f;
    float m_ambient = 0.5f;
};
