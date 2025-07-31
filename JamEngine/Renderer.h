#pragma once
#include "RendererCommons.h"

namespace jam
{

class WindowResizeEvent;
class Event;
class Texture2D;

class Renderer
{
public:
    Renderer() = delete;

    // core interface
    static void Initialize();
    static void OnEvent(const Event& _event);

    // swap chain interface
    static void      Present(bool _bVSync);
    static Texture2D GetBackBufferTexture();

    static NODISCARD ID3D11Device*        GetDevice();
    static NODISCARD ID3D11DeviceContext* GetDeviceContext();
    static UInt32                         GetMaxMultisampleQuality(DXGI_FORMAT _format, UInt32 _sampleCount);

    // d3d factories
    static void CreateBuffer(const D3D11_BUFFER_DESC& _desc, const std::optional<BufferInitializeData>& _initializeData, ID3D11Buffer** _out_pBuffer);
    static void CreateTexture2D(const D3D11_TEXTURE2D_DESC& _desc, const std::optional<Texture2DInitializeData>& _initializeData, ID3D11Texture2D** _out_pTexture);

    static void CreateShaderResourceView(ID3D11Resource* _pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* _pDesc, ID3D11ShaderResourceView** _out_pSRV);
    static void CreateRenderTargetView(ID3D11Resource* _pResource, const D3D11_RENDER_TARGET_VIEW_DESC* _pDesc, ID3D11RenderTargetView** _out_pRTV);
    static void CreateDepthStencilView(ID3D11Resource* _pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* _pDesc, ID3D11DepthStencilView** _out_pDSV);

    static void CreateInputLayout(std::span<const D3D11_INPUT_ELEMENT_DESC> _inputElements, ID3DBlob* _pVertexShaderBlob, ID3D11InputLayout** _out_pInputLayout);
    static void CreateInputLayout(std::span<const D3D11_INPUT_ELEMENT_DESC> _inputElements, ID3D11InputLayout** _out_pInputLayout);
    static void CreateVertexShader(const ShaderCreationData& _data, ID3D11VertexShader** _out_pVertexShader);
    static void CreatePixelShader(const ShaderCreationData& _data, ID3D11PixelShader** _out_pPixelShader);
    static void CreateHullShader(const ShaderCreationData& _data, ID3D11HullShader** _out_pHullShader);
    static void CreateDomainShader(const ShaderCreationData& _data, ID3D11DomainShader** _out_pDomainShader);
    static void CreateGeometryShader(const ShaderCreationData& _data, ID3D11GeometryShader** _out_pGeometryShader);
    static void CreateComputeShader(const ShaderCreationData& _data, ID3D11ComputeShader** _out_pComputeShader);

    static void CreateSamplerState(const D3D11_SAMPLER_DESC& _desc, ID3D11SamplerState** _out_pSamplerState);
    static void CreateBlendState(const D3D11_BLEND_DESC& _desc, ID3D11BlendState** _out_pBlendState);
    static void CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& _desc, ID3D11DepthStencilState** _out_pDepthStencilState);
    static void CreateRasterizerState(const D3D11_RASTERIZER_DESC& _desc, ID3D11RasterizerState** _out_pRasterizerState);

    // pipeline interface
    static void BindTopology(D3D11_PRIMITIVE_TOPOLOGY _topology);
    static void BindVertexBuffer(ID3D11Buffer* _pVertexBuffer, UInt32 _stride);
    static void BindIndexBuffer(ID3D11Buffer* _pIndexBuffer);

    static void BindInputLayout(ID3D11InputLayout* _pInputLayout);
    static void BindVertexShader(ID3D11VertexShader* _pVertexShader);
    static void BindPixelShader(ID3D11PixelShader* _pPixelShader);
    static void BindGeometryShader(ID3D11GeometryShader* _pGeometryShader);
    static void BindHullShader(ID3D11HullShader* _pHullShader);
    static void BindDomainShader(ID3D11DomainShader* _pDomainShader);
    static void BindComputeShader(ID3D11ComputeShader* _pComputeShader);

    static void BindBlendState(ID3D11BlendState* _pBlendState, const FLOAT _blendFactor[4]);
    static void BindDepthStencilState(ID3D11DepthStencilState* _pDepthStencilState, UINT _stencilRef = 0);
    static void BindRasterizerState(ID3D11RasterizerState* _pRasterizerState);

    static void BindSamplerStates(eShader _shader, UInt32 _slot, std::span<ID3D11SamplerState* const> _samplers);
    static void BindShaderResourceViews(eShader _shader, UInt32 _slot, std::span<ID3D11ShaderResourceView* const> _resources);
    static void BindConstantBuffers(eShader _shader, UInt32 _slot, std::span<ID3D11Buffer* const> _buffers);

    static void BindViewports(std::span<const D3D11_VIEWPORT> _viewports);
    static void BindRenderTargetViews(std::span<ID3D11RenderTargetView* const> _renderTargets, ID3D11DepthStencilView* _pDSV);

    static void UnbindSamplerStates(eShader _shader, UInt32 _slot, UInt32 _count = 1);
    static void UnbindShaderResourceViews(eShader _shader, UInt32 _slot, UInt32 _count = 1);
    static void UnbindConstantBuffers(eShader _shader, UInt32 _slot, UInt32 _count = 1);
    static void UnbindRenderTargetViews();

    // draw
    static void Draw(UInt32 _vertexCount, UInt32 _startVertexLocation);
    static void DrawIndices(UInt32 _indexCount, UInt32 _startIndexLocation, Int32 _baseVertexLocation);
    static void DrawFullScreenQuad();

private:
    static void OnResize_(const WindowResizeEvent& _event);
};

}   // namespace jam