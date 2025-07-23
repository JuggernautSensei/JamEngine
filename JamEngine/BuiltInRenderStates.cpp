#include "pch.h"

#include "BuiltInRenderStates.h"

#include "RenderStates.h"

namespace
{

struct BuiltInRenederStates
{
    // Sampler states
    jam::SamplerState samplerPointClamp;
    jam::SamplerState samplerPointWrap;
    jam::SamplerState samplerLinearClamp;
    jam::SamplerState samplerLinearWrap;
    jam::SamplerState samplerLinearAnisotropic4Wrap;
    jam::SamplerState samplerShadowComparisonLinearWrap;

    // Depth stencil states
    jam::DepthStencilState depthStencilNoDepthTest;
    jam::DepthStencilState depthStencilDepthTest;
    jam::DepthStencilState depthStencilReadOnlyDepthTest;

    // Blend states
    jam::BlendState blendOpacue;
    jam::BlendState blendAlphaBlend;
    jam::BlendState blendAdditive;

    // Rasterizer states
    jam::RasterizerState rasterizerSolidCullNone;
    jam::RasterizerState rasterizerSolidCullFront;
    jam::RasterizerState rasterizerSolidCullBack;

    jam::RasterizerState rasterizerWireframeCullNone;
    jam::RasterizerState rasterizerWireframeCullFront;
    jam::RasterizerState rasterizerWireframeCullBack;
};

BuiltInRenederStates g_renderStates;

}   // namespace

namespace jam
{

SamplerState StatesManager::GetSamplerPointClamp()
{
    SamplerState& sampler = g_renderStates.samplerPointClamp;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
    }

    return g_renderStates.samplerPointClamp;
}

SamplerState StatesManager::GetSamplerPointWrap()
{
    SamplerState& sampler = g_renderStates.samplerPointWrap;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
    }

    return g_renderStates.samplerPointWrap;
}

SamplerState StatesManager::GetSamplerLinearClamp()
{
    SamplerState& sampler = g_renderStates.samplerLinearClamp;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
    }

    return g_renderStates.samplerLinearClamp;
}

SamplerState StatesManager::GetSamplerLinearWrap()
{
    SamplerState& sampler = g_renderStates.samplerLinearWrap;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
    }

    return g_renderStates.samplerLinearWrap;
}

SamplerState StatesManager::GetSamplerLinearAnisotropic4Wrap()
{
    SamplerState& sampler = g_renderStates.samplerLinearAnisotropic4Wrap;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, 4);
    }

    return g_renderStates.samplerLinearAnisotropic4Wrap;
}

SamplerState StatesManager::GetSamplerShadowComparisonLinearWrap()
{
    SamplerState& sampler = g_renderStates.samplerShadowComparisonLinearWrap;
    if (sampler.IsValid() == false)
    {
        sampler = SamplerState::Create(D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_LESS_EQUAL);
    }

    return g_renderStates.samplerShadowComparisonLinearWrap;
}

DepthStencilState StatesManager::GetDepthStencilNoDepthTest()
{
    DepthStencilState& depthStencil = g_renderStates.depthStencilNoDepthTest;
    if (depthStencil.IsValid() == false)
    {
        depthStencil = DepthStencilState::Create(false, false, D3D11_COMPARISON_ALWAYS);
    }

    return g_renderStates.depthStencilNoDepthTest;
}

DepthStencilState StatesManager::GetDepthStencilDepthTest()
{
    DepthStencilState& depthStencil = g_renderStates.depthStencilDepthTest;
    if (depthStencil.IsValid() == false)
    {
        depthStencil = DepthStencilState::Create(true, true, D3D11_COMPARISON_LESS);
    }

    return g_renderStates.depthStencilDepthTest;
}

DepthStencilState StatesManager::GetDepthStencilReadOnlyDepthTest()
{
    DepthStencilState& depthStencil = g_renderStates.depthStencilReadOnlyDepthTest;
    if (depthStencil.IsValid() == false)
    {
        depthStencil = DepthStencilState::Create(true, false, D3D11_COMPARISON_LESS);
    }

    return g_renderStates.depthStencilReadOnlyDepthTest;
}

BlendState StatesManager::GetBlendOpaque()
{
    BlendState& blend = g_renderStates.blendOpacue;
    if (blend.IsValid() == false)
    {
        blend = BlendState::Create(false, false, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
    }

    return g_renderStates.blendOpacue;
}

BlendState StatesManager::GetBlendAlphaBlend()
{
    BlendState& blend = g_renderStates.blendAlphaBlend;
    if (blend.IsValid() == false)
    {
        blend = BlendState::Create(false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
    }

    return g_renderStates.blendAlphaBlend;
}

BlendState StatesManager::GetBlendAdditive()
{
    BlendState& blend = g_renderStates.blendAdditive;
    if (blend.IsValid() == false)
    {
        blend = BlendState::Create(false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD);
    }

    return g_renderStates.blendAdditive;
}

RasterizerState StatesManager::GetRasterizerSolidCullNone()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerSolidCullNone;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_SOLID, D3D11_CULL_NONE, false);
    }

    return g_renderStates.rasterizerSolidCullNone;
}

RasterizerState StatesManager::GetRasterizerSolidCullFront()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerSolidCullFront;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_SOLID, D3D11_CULL_FRONT, false);
    }

    return g_renderStates.rasterizerSolidCullFront;
}

RasterizerState StatesManager::GetRasterizerSolidCullBack()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerSolidCullBack;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_SOLID, D3D11_CULL_BACK, false);
    }

    return g_renderStates.rasterizerSolidCullBack;
}

RasterizerState StatesManager::GetRasterizerWireframeCullNone()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerWireframeCullNone;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_WIREFRAME, D3D11_CULL_NONE, false);
    }

    return g_renderStates.rasterizerWireframeCullNone;
}

RasterizerState StatesManager::GetRasterizerWireframeCullFront()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerWireframeCullFront;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_WIREFRAME, D3D11_CULL_FRONT, false);
    }

    return g_renderStates.rasterizerWireframeCullFront;
}

RasterizerState StatesManager::GetRasterizerWireframeCullBack()
{
    RasterizerState& rasterizer = g_renderStates.rasterizerWireframeCullBack;
    if (rasterizer.IsValid() == false)
    {
        rasterizer = RasterizerState::Create(D3D11_FILL_WIREFRAME, D3D11_CULL_BACK, false);
    }

    return g_renderStates.rasterizerWireframeCullBack;
}

}   // namespace jam