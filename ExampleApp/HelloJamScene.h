#pragma once

class HelloJamScene : public Scene
{
    JAM_SCENE(HelloJamScene);

public:
    void OnEnter() override;
    void OnExit() override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;
    void OnResize(Int32 _width, Int32 _height) override;

private:
    FrameBufferHandle m_fbh      = k_invalidHandle;
    TextureHandle     m_renderTh = k_invalidHandle;
    TextureHandle     m_depthTh  = k_invalidHandle;
    TextureHandle     m_cubeTh   = k_invalidHandle;

    VertexBufferHandle   m_vbh          = k_invalidHandle;
    IndexBufferHandle    m_ibh          = k_invalidHandle;
    ConstantBufferHandle m_transformCbh = k_invalidHandle;
    ConstantBufferHandle m_globalCbh  = k_invalidHandle;

    ProgramHandle m_program = k_invalidHandle;

    RenderState   m_gfxState        = RenderState::DepthTest();
    BlendState m_blendState      = BlendState::Opaque();
    float      m_rotSpeedRad     = ToRad(90.f);
    bool       m_bAnimation      = true;
    bool       m_bTextureMapping = false;

    Int32                  m_numCubePerRow    = 10;
    static constexpr Int32 k_cubeCountPerRowMax = 500;
};
