#pragma once

namespace jam
{

class SamplerState;
class DepthStencilState;
class BlendState;
class RasterizerState;

class StateCollection
{
public:
    static const SamplerState& SamplerPointClamp();
    static const SamplerState& SamplerPointWrap();
    static const SamplerState& SamplerLinearClamp();
    static const SamplerState& SamplerLinearWrap();
    static const SamplerState& SamplerLinearAnisotropic4Wrap();
    static const SamplerState& SamplerShadowComparisonLinearWrap();

    static const DepthStencilState& DepthStencilNoDepthTest();
    static const DepthStencilState& DepthStencilDepthTest();
    static const DepthStencilState& DepthStencilReadOnlyDepthTest();

    static const BlendState& BlendOpaque();
    static const BlendState& BlendAlpha();
    static const BlendState& BlendAdditive();

    static const RasterizerState& RasterizerSolidCullNone();
    static const RasterizerState& RasterizerSolidCullFront();
    static const RasterizerState& RasterizerSolidCullBack();
    static const RasterizerState& RasterizerWireframeCullNone();
    static const RasterizerState& RasterizerWireframeCullFront();
    static const RasterizerState& RasterizerWireframeCullBack();
};

}   // namespace jam