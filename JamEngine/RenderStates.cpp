#include "pch.h"

#include "RenderStates.h"

#include "Renderer.h"

namespace jam
{

SamplerState SamplerState::Create(const D3D11_FILTER _filter, const D3D11_TEXTURE_ADDRESS_MODE _addressMode, const UInt32 _anisotropy, const D3D11_COMPARISON_FUNC _comparisonFunc, float _borderColor[4])
{
    constexpr float k_defaultBorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    return CreateEx(_filter, _addressMode, _addressMode, _addressMode, 0.0f, _anisotropy, _comparisonFunc, _borderColor ? _borderColor : k_defaultBorderColor, 0.0f, D3D11_FLOAT32_MAX);
}

SamplerState SamplerState::CreateEx(const D3D11_FILTER _filter, const D3D11_TEXTURE_ADDRESS_MODE _addressU, const D3D11_TEXTURE_ADDRESS_MODE _addressV, const D3D11_TEXTURE_ADDRESS_MODE _addressW, const float _mipLODBias, const UInt32 _maxAnisotropy, const D3D11_COMPARISON_FUNC _comparisonFunc, const float _borderColor[4], const float _minLOD, const float _maxLOD)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = _filter;
    samplerDesc.AddressU           = _addressU;
    samplerDesc.AddressV           = _addressV;
    samplerDesc.AddressW           = _addressW;
    samplerDesc.MipLODBias         = _mipLODBias;
    samplerDesc.MaxAnisotropy      = _maxAnisotropy;
    samplerDesc.ComparisonFunc     = _comparisonFunc;
    samplerDesc.BorderColor[0]     = _borderColor[0];
    samplerDesc.BorderColor[1]     = _borderColor[1];
    samplerDesc.BorderColor[2]     = _borderColor[2];
    samplerDesc.BorderColor[3]     = _borderColor[3];
    samplerDesc.MinLOD             = _minLOD;
    samplerDesc.MaxLOD             = _maxLOD;

    SamplerState samplerState;
    Renderer::CreateSamplerState(samplerDesc, samplerState.GetAddressOf());
    return samplerState;
}

auto SamplerState::Bind(const eShader _shader, const UInt32 _slot) const -> void
{
    ID3D11SamplerState* samplerStates[] = { m_pSamplerState.Get() };
    Renderer::SetSamplerStates(_shader, _slot, 1, samplerStates);
}

BlendState BlendState::Create(const bool _bAlphaToCoverageEnabled, const bool _blendEnable, const D3D11_BLEND _srcBlend, const D3D11_BLEND _destBlend, const D3D11_BLEND_OP _blendOp, const D3D11_BLEND _srcBlendAlpha, const D3D11_BLEND _destBlendAlpha, const D3D11_BLEND_OP _blendOpAlpha, const UINT8 _renderTargetWriteMask)
{
    D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
    renderTargetBlendDesc.BlendEnable           = _blendEnable;
    renderTargetBlendDesc.SrcBlend              = _srcBlend;
    renderTargetBlendDesc.DestBlend             = _destBlend;
    renderTargetBlendDesc.BlendOp               = _blendOp;
    renderTargetBlendDesc.SrcBlendAlpha         = _srcBlendAlpha;
    renderTargetBlendDesc.DestBlendAlpha        = _destBlendAlpha;
    renderTargetBlendDesc.BlendOpAlpha          = _blendOpAlpha;
    renderTargetBlendDesc.RenderTargetWriteMask = _renderTargetWriteMask;
    return CreateEx(_bAlphaToCoverageEnabled, false, &renderTargetBlendDesc, 1);
}

BlendState BlendState::CreateEx(const bool _bAlphaToCoverageEnabled, const bool _bIndependentBlendEnabled, const D3D11_RENDER_TARGET_BLEND_DESC* _pRenderTargetBlendDesc, const UInt32 _renderTargetBlendDescCount)
{
    D3D11_BLEND_DESC blendDesc       = {};
    blendDesc.AlphaToCoverageEnable  = _bAlphaToCoverageEnabled;
    blendDesc.IndependentBlendEnable = _bIndependentBlendEnabled;
    for (UInt32 i = 0; i < _renderTargetBlendDescCount; ++i)
    {
        blendDesc.RenderTarget[i] = _pRenderTargetBlendDesc[i];
    }

    BlendState blendState;
    Renderer::CreateBlendState(blendDesc, blendState.GetAddressOf());
    return blendState;
}

void BlendState::Bind(float _blendFactor[4]) const
{
    Renderer::SetBlendState(m_pBlendState.Get(), _blendFactor);
}

DepthStencilState DepthStencilState::Create(const bool _depthEnable, const bool _depthWrite, const D3D11_COMPARISON_FUNC _depthFunc)
{
    return CreateEx(_depthEnable, _depthWrite, _depthFunc, false, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS, 0xFF, 0xFF);
}

DepthStencilState DepthStencilState::CreateEx(const bool _depthEnable, const bool _depthWrite, const D3D11_COMPARISON_FUNC _depthFunc, const bool _stencilEnable, const D3D11_STENCIL_OP _frontFaceStencilFailOp, const D3D11_STENCIL_OP _frontFaceStencilDepthFailOp, const D3D11_STENCIL_OP _frontFaceStencilPassOp, const D3D11_COMPARISON_FUNC _frontFaceStencilFunc, const D3D11_STENCIL_OP _backFaceStencilFailOp, const D3D11_STENCIL_OP _backFaceStencilDepthFailOp, const D3D11_STENCIL_OP _backFaceStencilPassOp, const D3D11_COMPARISON_FUNC _backFaceStencilFunc, const UINT8 _stencilReadMask, const UINT8 _stencilWriteMask)
{
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    depthStencilDesc.DepthEnable                  = _depthEnable;
    depthStencilDesc.DepthWriteMask               = _depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc                    = _depthFunc;
    depthStencilDesc.StencilEnable                = _stencilEnable;
    depthStencilDesc.StencilReadMask              = _stencilReadMask;
    depthStencilDesc.StencilWriteMask             = _stencilWriteMask;
    depthStencilDesc.FrontFace.StencilFailOp      = _frontFaceStencilFailOp;
    depthStencilDesc.FrontFace.StencilDepthFailOp = _frontFaceStencilDepthFailOp;
    depthStencilDesc.FrontFace.StencilPassOp      = _frontFaceStencilPassOp;
    depthStencilDesc.FrontFace.StencilFunc        = _frontFaceStencilFunc;
    depthStencilDesc.BackFace.StencilFailOp       = _backFaceStencilFailOp;
    depthStencilDesc.BackFace.StencilDepthFailOp  = _backFaceStencilDepthFailOp;
    depthStencilDesc.BackFace.StencilPassOp       = _backFaceStencilPassOp;
    depthStencilDesc.BackFace.StencilFunc         = _backFaceStencilFunc;

    DepthStencilState depthStencilState;
    Renderer::CreateDepthStencilState(depthStencilDesc, depthStencilState.GetAddressOf());
    return depthStencilState;
}

void DepthStencilState::Bind(const UInt32 _stencilRef) const
{
    Renderer::SetDepthStencilState(m_pDepthStencilState.Get(), _stencilRef);
}

RasterizerState RasterizerState::Create(const D3D11_FILL_MODE _fillMode, const D3D11_CULL_MODE _cullMode, const bool _bMultiSampleEnable)
{
    return CreateEx(_fillMode, _cullMode, false, 0, 0.0f, 0.0f, true, false, _bMultiSampleEnable, false);
}

RasterizerState RasterizerState::CreateEx(const D3D11_FILL_MODE _fillMode, const D3D11_CULL_MODE _cullMode, const bool _frontCounterClockwise, const int _depthBias, const float _depthBiasClamp, const float _slopeScaledDepthBias, const bool _depthClipEnable, const bool _scissorEnable, const bool _multisampleEnable, const bool _antialiasedLineEnable)
{
    D3D11_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = _fillMode;
    rasterizerDesc.CullMode              = _cullMode;
    rasterizerDesc.FrontCounterClockwise = _frontCounterClockwise;
    rasterizerDesc.DepthBias             = _depthBias;
    rasterizerDesc.DepthBiasClamp        = _depthBiasClamp;
    rasterizerDesc.SlopeScaledDepthBias  = _slopeScaledDepthBias;
    rasterizerDesc.DepthClipEnable       = _depthClipEnable;
    rasterizerDesc.ScissorEnable         = _scissorEnable;
    rasterizerDesc.MultisampleEnable     = _multisampleEnable;
    rasterizerDesc.AntialiasedLineEnable = _antialiasedLineEnable;

    RasterizerState rasterizerState;
    Renderer::CreateRasterizerState(rasterizerDesc, rasterizerState.GetAddressOf());
    return rasterizerState;
}

void RasterizerState::Bind() const
{
    Renderer::SetRasterizerState(m_pRasterizerState.Get());
}

}   // namespace jam