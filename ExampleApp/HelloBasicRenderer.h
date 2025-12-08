#pragma once

class HelloBasicRenderer : public Scene
{
    JAM_SCENE(HelloBasicRenderer);

public:
    void                        OnRender(const RenderView& _view) override;
    [[nodiscard]] TextureHandle GetDepthTexture() const override;
};
