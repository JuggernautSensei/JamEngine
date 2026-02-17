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
    FrameBufferHandle m_fbh      = k_nullHandle;
    TextureHandle     m_renderTh = k_nullHandle;
    TextureHandle     m_depthTh  = k_nullHandle;
    TextureHandle     m_cubeTh   = k_nullHandle;

    VertexBufferHandle   m_vbh          = k_nullHandle;
    IndexBufferHandle    m_ibh          = k_nullHandle;
    ConstantBufferHandle m_transformCbh = k_nullHandle;
    ConstantBufferHandle m_globalCbh  = k_nullHandle;

    ProgramHandle m_program = k_nullHandle;

    RenderState   m_gfxState        = RenderState::k_depthTest;
    BlendState m_blendState      = BlendState::k_opaque;
    float      m_rotSpeedRad     = ToRad(90.f);
    bool       m_bAnimation      = true;
    bool       m_bTextureMapping = false;

    Int32                  m_numCubePerRow    = 10;
    static constexpr Int32 k_cubeCountPerRowMax = 500;
};
