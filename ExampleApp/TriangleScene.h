#pragma once

class TriangleScene : public Scene
{
    JAM_SCENE(TriangleScene);

public:
    void OnEnter() override;
    void OnExit() override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    VertexBufferHandle m_vbh     = k_invalidHandle;
    IndexBufferHandle  m_ibh     = k_invalidHandle;
    ProgramHandle      m_program = k_invalidHandle;

    ConstantBufferHandle m_cbh = k_invalidHandle;
    Vector3                 m_offset = Vector3::Zero;
};
