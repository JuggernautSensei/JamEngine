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
    VertexBufferHandle m_vbh     = k_nullHandle;
    IndexBufferHandle  m_ibh     = k_nullHandle;
    ProgramHandle      m_program = k_nullHandle;

    ConstantBufferHandle m_cbh = k_nullHandle;
    Vec3                 m_offset = Vec3::Zero;
};
