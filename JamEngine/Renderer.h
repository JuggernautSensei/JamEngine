#pragma once
#include "RendererCommons.h"

namespace jam
{

class WindowResizeEvent;
class Event;

struct RendererInitializeInfo
{
    HWND windowHandle = NULL;
    int  width        = 800;
    int  height       = 600;
};

class Renderer
{
public:
    // core interface
    static void Initialize(const RendererInitializeInfo& _info);
    static void OnEvent(const Event& _event);

    static NODISCARD ID3D11Device*        GetDevice();
    static NODISCARD ID3D11DeviceContext* GetDeviceContext();
    static UInt32                         GetMaxMultisampleQuality(DXGI_FORMAT _format, UInt32 _sampleCount);

    // d3d factories
    static void CreateBuffer(const D3D11_BUFFER_DESC& _desc, const void* _pInitialData_orNull, ID3D11Buffer** _out_pBuffer);
    static void CreateTexture2D(const D3D11_TEXTURE2D_DESC& _desc, const void* _pInitialData_orNull, UInt32 _initialiDataPitch_orZero, ID3D11Texture2D** _out_pTexture);

    static void CreateShaderResourceView(ID3D11Resource* _pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* _pDesc, ID3D11ShaderResourceView** _out_pSRV);
    static void CreateRenderTargetView(ID3D11Resource* _pResource, const D3D11_RENDER_TARGET_VIEW_DESC* _pDesc, ID3D11RenderTargetView** _out_pRTV);
    static void CreateDepthStencilView(ID3D11Resource* _pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* _pDesc, ID3D11DepthStencilView** _out_pDSV);

    static void CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* _pInputElementDescs, UINT _numElements, ID3D11InputLayout** _out_pInputLayout);
    static void CreateVertexShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11VertexShader** _out_pVertexShader);
    static void CreatePixelShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11PixelShader** _out_pPixelShader);
    static void CreateHullShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11HullShader** _out_pHullShader);
    static void CreateDomainShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11DomainShader** _out_pDomainShader);
    static void CreateGeometryShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11GeometryShader** _out_pGeometryShader);
    static void CreateComputeShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11ComputeShader** _out_pComputeShader);
    static void CreateSamplerState(const D3D11_SAMPLER_DESC& _desc, ID3D11SamplerState** _out_pSamplerState);

    static void CreateBlendState(const D3D11_BLEND_DESC& _desc, ID3D11BlendState** _out_pBlendState);
    static void CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& _desc, ID3D11DepthStencilState** _out_pDepthStencilState);
    static void CreateRasterizerState(const D3D11_RASTERIZER_DESC& _desc, ID3D11RasterizerState** _out_pRasterizerState);

    // pipeline interface
    static void SetVertexBuffer(ID3D11Buffer* _pVertexBuffer, UInt32 _stride);
    static void SetIndexBuffer(ID3D11Buffer* _pIndexBuffer, bool _bUseExtendedIndex);

    static void SetInputLayout(ID3D11InputLayout* _pInputLayout);
    static void SetVertexShader(ID3D11VertexShader* _pVertexShader);
    static void SetPixelShader(ID3D11PixelShader* _pPixelShader);
    static void SetGeometryShader(ID3D11GeometryShader* _pGeometryShader);
    static void SetHullShader(ID3D11HullShader* _pHullShader);
    static void SetDomainShader(ID3D11DomainShader* _pDomainShader);
    static void SetComputeShader(ID3D11ComputeShader* _pComputeShader);

    static void SetBlendState(ID3D11BlendState* _pBlendState, FLOAT _blendFactor[4] = nullptr);
    static void SetDepthStencilState(ID3D11DepthStencilState* _pDepthStencilState, UINT _stencilRef = 0);
    static void SetRasterizerState(ID3D11RasterizerState* _pRasterizerState);

    static void SetSamplerStates(eShader _shader, UInt32 _slot, UInt32 _samplerCount, ID3D11SamplerState* const* _ppSamplerState);
    static void SetShaderResourceViews(eShader _shader, UInt32 _slot, UInt32 _srvCount, ID3D11ShaderResourceView* const* _ppSRV);
    static void SetConstantBuffers(eShader _shader, UInt32 _slot, UInt32 _cbCount, ID3D11Buffer* const* _ppCB);

    static void SetViewports(UInt32 _numViewports, const D3D11_VIEWPORT* _pViewports);
    static void SetRenderTargetViews(UInt32 _rtvCount, ID3D11RenderTargetView* const* _ppRTV, ID3D11DepthStencilView* _pDSV = nullptr);

    // draw
    static void Draw(UInt32 _vertexCount, UInt32 _startVertexLocation);
    static void DrawIndices(UInt32 _indexCount, UInt32 _startIndexLocation, Int32 _baseVertexLocation);

private:
    static void OnResize_(Int32 _width, Int32 _height);
};

}   // namespace jam