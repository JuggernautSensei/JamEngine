#pragma once

class HelloBasicRenderer : public Scene
{
    JAM_SCENE(HelloBasicRenderer);

public:
    void OnEnter() override;
    void OnExit() override;
    void OnUpdate(float _deltaSec) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;
    void OnResize(Int32 _width, Int32 _height) override;

private:
    void Resize_(Int32 _width, Int32 _height);
    void CompileGraph_(Int32 _width, Int32 _height);

    FrameBufferHandle m_offscreenFbh = k_invalidHandle;
    TextureHandle     m_offscreenTh  = k_invalidHandle;

    FrameBufferHandle m_gbufferFbh                = k_invalidHandle;
    TextureHandle     m_albedoAoTh                = k_invalidHandle;
    TextureHandle     m_normalRoughnessMetallicTh = k_invalidHandle;
    TextureHandle     m_additionalLightTh         = k_invalidHandle;
    TextureHandle     m_depthTh                   = k_invalidHandle;

    Viewport    m_fullScrennVp = {};
    Environment m_env;
};
