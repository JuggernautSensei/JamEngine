#include "pch.h"

#include "StateCollection.h"

#include "RenderStates.h"

namespace jam
{

const SamplerState& StateCollection::SamplerPointClamp()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
        return sampler;
    }();

    return s_sampler;
}

const SamplerState& StateCollection::SamplerPointWrap()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
        return sampler;
    }();
    return s_sampler;
}

const SamplerState& StateCollection::SamplerLinearClamp()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
        return sampler;
    }();

    return s_sampler;
}

const SamplerState& StateCollection::SamplerLinearWrap()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
        return sampler;
    }();
    return s_sampler;
}

const SamplerState& StateCollection::SamplerLinearAnisotropic4Wrap()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, 4);
        return sampler;
    }();
    return s_sampler;
}

const SamplerState& StateCollection::SamplerShadowComparisonLinearWrap()
{
    static SamplerState s_sampler = []
    {
        SamplerState sampler;
        sampler.Initialize(D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_LESS_EQUAL);
        return sampler;
    }();
    return s_sampler;
}

const DepthStencilState& StateCollection::DepthStencilNoDepthTest()
{
    static DepthStencilState s_depthStencil = []
    {
        DepthStencilState depthStencil;
        depthStencil.Initialize(false, false, D3D11_COMPARISON_ALWAYS);
        return depthStencil;
    }();
    return s_depthStencil;
}

const DepthStencilState& StateCollection::DepthStencilDepthTest()
{
    static DepthStencilState s_depthStencil = []
    {
        DepthStencilState depthStencil;
        depthStencil.Initialize(true, true, D3D11_COMPARISON_LESS);
        return depthStencil;
    }();
    return s_depthStencil;
}

const DepthStencilState& StateCollection::DepthStencilReadOnlyDepthTest()
{
    static DepthStencilState s_depthStencil = []
    {
        DepthStencilState depthStencil;
        depthStencil.Initialize(true, false, D3D11_COMPARISON_LESS);
        return depthStencil;
    }();
    return s_depthStencil;
}

const BlendState& StateCollection::BlendOpaque()
{
    static BlendState s_blend = []
    {
        BlendState blend;
        blend.Initialize(false, false, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
        return blend;
    }();
    return s_blend;
}

const BlendState& StateCollection::BlendAlpha()
{
    static BlendState s_blend = []
    {
        BlendState blend;
        blend.Initialize(false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
        return blend;
    }();
    return s_blend;
}

const BlendState& StateCollection::BlendAdditive()
{
    static BlendState s_blend = []
    {
        BlendState blend;
        blend.Initialize(false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
        return blend;
    }();
    return s_blend;
}

const RasterizerState& StateCollection::RasterizerSolidCullNone()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_SOLID, D3D11_CULL_NONE, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

const RasterizerState& StateCollection::RasterizerSolidCullFront()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_SOLID, D3D11_CULL_FRONT, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

const RasterizerState& StateCollection::RasterizerSolidCullBack()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_SOLID, D3D11_CULL_BACK, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

const RasterizerState& StateCollection::RasterizerWireframeCullNone()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_WIREFRAME, D3D11_CULL_NONE, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

const RasterizerState& StateCollection::RasterizerWireframeCullFront()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_WIREFRAME, D3D11_CULL_FRONT, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

const RasterizerState& StateCollection::RasterizerWireframeCullBack()
{
    static RasterizerState s_rasterizer = []
    {
        RasterizerState rasterizer;
        rasterizer.Initialize(D3D11_FILL_WIREFRAME, D3D11_CULL_BACK, false);
        return rasterizer;
    }();
    return s_rasterizer;
}

}   // namespace jam