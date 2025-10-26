#pragma once

class HelloCubeScene : public Scene
{
    JAM_SCENE(HelloCubeScene);

public:
    HelloCubeScene();
    ~HelloCubeScene() override;

    void OnUpdate(float _deltaSec) override;
    void OnRender(const SceneView& _view) override;
    void OnImGuiRender() override;

private:
    float m_dt = 0.f;

    VertexBufferHandle   m_cubeVbh = k_invalidHandle;
    IndexBufferHandle    m_cubeIbh = k_invalidHandle;
    ProgramHandle        m_program = k_invalidHandle;
    ConstantBufferHandle m_cbh     = k_invalidHandle;

    Transform m_cubeTransform = {};
    float     m_rotSpeed      = ToRad(45.f);

    Vec3  m_eyePos = Vec3 { 0.f, 0.f, -3.f };
    Vec3  m_eyeDir = Vec3 { 0.f, 0.f, 1.f };
    float m_fov    = ToRad(60.f);
    float m_nearZ  = 0.1f;
    float m_farZ   = 100.f;
    float m_aspect = 16.f / 9.f;
};