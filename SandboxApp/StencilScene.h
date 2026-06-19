#pragma once
#include "FlyingCamera.h"

class StencilScene : public Scene
{
    JAM_SCENE_BODY(StencilScene);

public:
    StencilScene();
    ~StencilScene() override;

    StencilScene(const StencilScene&)                = delete;
    StencilScene& operator=(const StencilScene&)     = delete;
    StencilScene(StencilScene&&) noexcept            = delete;
    StencilScene& operator=(StencilScene&&) noexcept = delete;

    void OnUpdate(float _deltaSec) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMeshes_();

    RenderGraph  m_graph  = {};
    FlyingCamera m_camera = {};
};


// 수행할 실습
// 1. plarnar reflection
// 2. stencil outline
// 3. stencil shadow 