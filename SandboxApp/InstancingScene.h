#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

class InstancingScene : public Scene
{
    JAM_SCENE_BODY(InstancingScene);

public:
    InstancingScene();
    ~InstancingScene() noexcept override;

    InstancingScene(const InstancingScene&)                = delete;
    InstancingScene& operator=(const InstancingScene&)     = delete;
    InstancingScene(InstancingScene&&) noexcept            = delete;
    InstancingScene& operator=(InstancingScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMeshes_();
    void InitRenderGraph_();
    void Submit_();

    RenderGraph m_graph = {};

    ConstantBufferHandle m_entityNoInstancingCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh                = kInvalidHandle;

    VertexBufferHandle m_vbh = kInvalidHandle;
    IndexBufferHandle  m_ibh = kInvalidHandle;

    VertexBufferHandle m_instanceVbh             = kInvalidHandle;
    bool               m_bDoInstancing           = true;
    Int32              m_numLastRenderedEntities = 0;
    Int32              m_numEntitiesPerRow       = 10;

    bool  m_bAnimate = true;
    float m_dRot     = ToRad(45.f);
    float m_rot      = 0.f;

    FlyingCamera m_camera = {};
};
