#pragma once
#include "FlyingCamera.h"

class MultiVertexStreamScene : public Scene
{
    JAM_SCENE_BODY(MultiVertexStreamScene);

public:
    MultiVertexStreamScene();
    ~MultiVertexStreamScene() noexcept override;

    MultiVertexStreamScene(const MultiVertexStreamScene&)                = delete;
    MultiVertexStreamScene& operator=(const MultiVertexStreamScene&)     = delete;
    MultiVertexStreamScene(MultiVertexStreamScene&&) noexcept            = delete;
    MultiVertexStreamScene& operator=(MultiVertexStreamScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    ProgramHandle m_ph = kInvalidHandle;

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;

    FrameBufferHandle m_fbh    = {};
    TextureHandle     m_colorTexh = {};   // not owned

    bool  m_bAnimate = true;
    float m_dRot     = ToRad(45.f);
    float m_rot      = 0.f;

    VertexBufferHandle m_posVbh   = kInvalidHandle;
    VertexBufferHandle m_colorVbh = kInvalidHandle;
    IndexBufferHandle  m_ibh      = kInvalidHandle;

    FlyingCamera m_camera = {};
};
