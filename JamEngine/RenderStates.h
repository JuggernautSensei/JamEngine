#pragma once
#include "RendererCommons.h"

namespace jam
{

class SamplerState
{
public:
    static NODISCARD SamplerState Create(D3D11_FILTER _filter, D3D11_TEXTURE_ADDRESS_MODE _addressMode, UInt32 _anisotropy = 0, D3D11_COMPARISON_FUNC _comparisonFunc = D3D11_COMPARISON_NEVER, float _borderColor[4] = nullptr);
    static NODISCARD SamplerState CreateEx(D3D11_FILTER _filter, D3D11_TEXTURE_ADDRESS_MODE _addressU, D3D11_TEXTURE_ADDRESS_MODE _addressV, D3D11_TEXTURE_ADDRESS_MODE _addressW, float _mipLODBias, UInt32 _maxAnisotropy, D3D11_COMPARISON_FUNC _comparisonFunc, const float _borderColor[4], float _minLOD, float _maxLOD);

    void Bind(eShader _shader, UInt32 _slot) const;

    NODISCARD ID3D11SamplerState*  Get() const { return m_pSamplerState.Get(); }
    NODISCARD ID3D11SamplerState** GetAddressOf() { return m_pSamplerState.GetAddressOf(); }

private:
    ComPtr<ID3D11SamplerState> m_pSamplerState;
};

class BlendState
{
public:
    static NODISCARD BlendState Create(bool _bAlphaToCoverageEnabled, bool _blendEnable, D3D11_BLEND _srcBlend, D3D11_BLEND _destBlend, D3D11_BLEND_OP _blendOp, D3D11_BLEND _srcBlendAlpha, D3D11_BLEND _destBlendAlpha, D3D11_BLEND_OP _blendOpAlpha, UINT8 _renderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL);
    static NODISCARD BlendState CreateEx(bool _bAlphaToCoverageEnabled, bool _bIndependentBlendEnabled, const D3D11_RENDER_TARGET_BLEND_DESC* _pRenderTargetBlendDesc, UInt32 _renderTargetBlendDescCount);

    void Bind(float _blendFactor[4] = nullptr) const;

    NODISCARD ID3D11BlendState*  Get() const { return m_pBlendState.Get(); }
    NODISCARD ID3D11BlendState** GetAddressOf() { return m_pBlendState.GetAddressOf(); }

private:
    ComPtr<ID3D11BlendState> m_pBlendState;
};

class DepthStencilState
{
public:
    static NODISCARD DepthStencilState Create(bool _depthEnable, bool _depthWrite, D3D11_COMPARISON_FUNC _depthFunc);
    static NODISCARD DepthStencilState CreateEx(bool _depthEnable, bool _depthWrite, D3D11_COMPARISON_FUNC _depthFunc, bool _stencilEnable, D3D11_STENCIL_OP _frontFaceStencilFailOp, D3D11_STENCIL_OP _frontFaceStencilDepthFailOp, D3D11_STENCIL_OP _frontFaceStencilPassOp, D3D11_COMPARISON_FUNC _frontFaceStencilFunc, D3D11_STENCIL_OP _backFaceStencilFailOp, D3D11_STENCIL_OP _backFaceStencilDepthFailOp, D3D11_STENCIL_OP _backFaceStencilPassOp, D3D11_COMPARISON_FUNC _backFaceStencilFunc, UINT8 _stencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK, UINT8 _stencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK);

    void Bind(UInt32 _stencilRef = 0) const;

    NODISCARD ID3D11DepthStencilState*  Get() const { return m_pDepthStencilState.Get(); }
    NODISCARD ID3D11DepthStencilState** GetAddressOf() { return m_pDepthStencilState.GetAddressOf(); }

private:
    ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;
};

class RasterizerState
{
public:
    static NODISCARD RasterizerState Create(D3D11_FILL_MODE _fillMode, D3D11_CULL_MODE _cullMode, bool _bMultiSampleEnable);
    static NODISCARD RasterizerState CreateEx(D3D11_FILL_MODE _fillMode, D3D11_CULL_MODE _cullMode, bool _frontCounterClockwise, int _depthBias, float _depthBiasClamp, float _slopeScaledDepthBias, bool _depthClipEnable, bool _scissorEnable, bool _multisampleEnable, bool _antialiasedLineEnable);

    void Bind() const;

    NODISCARD ID3D11RasterizerState*  Get() const { return m_pRasterizerState.Get(); }
    NODISCARD ID3D11RasterizerState** GetAddressOf() { return m_pRasterizerState.GetAddressOf(); }

private:
    ComPtr<ID3D11RasterizerState> m_pRasterizerState;
};

}   // namespace jam