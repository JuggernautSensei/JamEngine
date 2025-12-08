#include "pch.h"

#include "HelloBasicRenderer.h"

void HelloBasicRenderer::OnRender(const RenderView& _view)
{
    BasicRenderer::Render(_view, *this);
}

TextureHandle HelloBasicRenderer::GetDepthTexture() const
{
    return BasicRenderer::GetDepthTexture();
}
