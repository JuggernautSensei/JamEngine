#pragma once

namespace jam
{

class SamplerState;
class DepthStencilState;
class BlendState;
class RasterizerState;

class StatesManager
{
public:
    static SamplerState GetSamplerPointClamp();
    static SamplerState GetSamplerPointWrap();
    static SamplerState GetSamplerLinearClamp();
    static SamplerState GetSamplerLinearWrap();
    static SamplerState GetSamplerLinearAnisotropic4Wrap();
    static SamplerState GetSamplerShadowComparisonLinearWrap();

    static DepthStencilState GetDepthStencilNoDepthTest();
    static DepthStencilState GetDepthStencilDepthTest();
    static DepthStencilState GetDepthStencilReadOnlyDepthTest();

    static BlendState GetBlendOpaque();
    static BlendState GetBlendAlphaBlend();
    static BlendState GetBlendAdditive();

    static RasterizerState GetRasterizerSolidCullNone();
    static RasterizerState GetRasterizerSolidCullFront();
    static RasterizerState GetRasterizerSolidCullBack();
    static RasterizerState GetRasterizerWireframeCullNone();
    static RasterizerState GetRasterizerWireframeCullFront();
    static RasterizerState GetRasterizerWireframeCullBack();
};

}   // namespace jam