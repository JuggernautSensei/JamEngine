#include "pch.h"

#include "Viewport.h"

#include "Renderer.h"

namespace jam
{

Viewport::Viewport(const float _x, const float _y, const float _width, const float _height, const float _minDepth, const float _maxDepth)
    : x(_x)
    , y(_y)
    , width(_width)
    , height(_height)
    , minDepth(_minDepth)
    , maxDepth(_maxDepth)
{
}

void Viewport::Bind()
{
    const D3D11_VIEWPORT viewports[] = { Get() };
    Renderer::SetViewports(1, viewports);
}

}   // namespace jam