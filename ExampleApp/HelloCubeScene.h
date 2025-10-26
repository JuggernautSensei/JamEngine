#pragma once

class HelloCubeScene : public Scene
{
    JAM_SCENE(HelloCubeScene);

public:
    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float _deltaSec) override;
    void OnRender(const SceneView& _view) override;
    void OnImGuiRender() override;
    void OnResize(Int32 _width, Int32 _height) override;

private:
    void Resize(Int32 _width, Int32 _height);

    float m_dt = 0.f;

    FrameBufferHandle m_sceneFbh        = k_invalidHandle;
    TextureHandle     m_sceneRenderTexh = k_invalidHandle;
    TextureHandle     m_sceneDepthTexh  = k_invalidHandle;
    Resolution        m_sceneResoultion = { 1600, 900 };

    VertexBufferHandle   m_cubeVbh = k_invalidHandle;
    IndexBufferHandle    m_cubeIbh = k_invalidHandle;
    ProgramHandle        m_program = k_invalidHandle;
    ConstantBufferHandle m_cbh     = k_invalidHandle;

    Transform m_cubeTransform = {};
    float     m_rotSpeed      = ToRad(45.f);

    Entity m_cmrEntity = Entity::s_null;
};
