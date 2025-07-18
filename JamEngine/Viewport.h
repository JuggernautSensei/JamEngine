#pragma once

namespace jam
{

struct Viewport
{
    Viewport() = default;
    Viewport(float _x, float _y, float _width, float _height, float _minDepth = 0.0f, float _maxDepth = 1.0f);

    NODISCARD D3D11_VIEWPORT Get() { return reinterpret_cast<D3D11_VIEWPORT&>(*this); }
    void                     Bind();

    float x        = 0.f;
    float y        = 0.f;
    float width    = 0.f;
    float height   = 0.f;
    float minDepth = 0.f;
    float maxDepth = 1.f;
};

}   // namespace jam