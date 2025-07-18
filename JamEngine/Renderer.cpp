#include "pch.h"

#include "Renderer.h"

#include "Application.h"
#include "Event.h"
#include "ShaderCompiler.h"
#include "WindowsUtilities.h"

namespace
{

struct RendererContext
{
    jam::ComPtr<ID3D11Device>        pDevice;
    jam::ComPtr<ID3D11DeviceContext> pDeviceContext;
    jam::ComPtr<IDXGISwapChain1>     pSwapChain;
};

RendererContext g_renderer;

}   // namespace

namespace jam
{

void Renderer::Initialize(const RendererInitializeInfo& _info)
{
    HRESULT hr;

    // create device and device context
    {
        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        UINT d3dFlags = 0;
#ifdef _DEBUG
        d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            d3dFlags,
            featureLevels,
            std::size(featureLevels),
            D3D11_SDK_VERSION,
            g_renderer.pDevice.GetAddressOf(),
            nullptr,
            g_renderer.pDeviceContext.GetAddressOf());

        if (FAILED(hr))
        {
            JAM_CRASH("Failed to create D3D11 device. HRESULT: {}", GetSystemErrorMessage(hr));
        }
    }

    // create swap chain
    {
        ComPtr<IDXGIDevice2> dxgiDevice2;
        hr = g_renderer.pDevice.As(&dxgiDevice2);
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIDevice1 from D3D11 device. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        ComPtr<IDXGIAdapter> dxgiAdapter;
        hr = dxgiDevice2->GetParent(IID_PPV_ARGS(dxgiAdapter.GetAddressOf()));
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIAdapter1 from IDXGIDevice1. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        ComPtr<IDXGIFactory2> dxgiFactory2;
        hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory2.GetAddressOf()));
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIFactory1 from IDXGIAdapter1. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width              = _info.width;
        swapChainDesc.Height             = _info.height;
        swapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo             = false;
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount        = 2;   // double buffering
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_PREMULTIPLIED;

        hr = dxgiFactory2->CreateSwapChainForHwnd(g_renderer.pDevice.Get(),
                                                  _info.windowHandle,
                                                  &swapChainDesc,
                                                  nullptr,
                                                  nullptr,
                                                  g_renderer.pSwapChain.GetAddressOf());

        if (FAILED(hr))
        {
            JAM_CRASH("Failed to create swap chain. HRESULT: {}", GetSystemErrorMessage(hr));
        }
    }
}

void Renderer::OnEvent(const Event& _event)
{
    // do not use event dispatcher
    if (_event.IsHandled())
    {
        JAM_ASSERT(0, "Event '{}' is already handled.", _event.GetName());

        // hand operated
        jam::Application& app    = GetApplication();
        const Window&     window = app.GetWindow();
        auto [width, height]     = window.GetWindowSize();
        OnResize_(width, height);
    }
    else
    {
        // use event dispatcher
        if (_event.GetHash() == WindowResizeEvent::k_staticHash)
        {
            // this is allowed casting
            const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(_event);
            OnResize_(resizeEvent.GetWidth(), resizeEvent.GetHeight());
        }
    }
}

ID3D11Device* Renderer::GetDevice()
{
    return g_renderer.pDevice.Get();
}

ID3D11DeviceContext* Renderer::GetDeviceContext()
{
    return g_renderer.pDeviceContext.Get();
}

UInt32 Renderer::GetMaxMultisampleQuality(const DXGI_FORMAT _format, const UInt32 _sampleCount)
{
    JAM_ASSERT(_sampleCount > 0 && _sampleCount <= 32, "Sample count must be between 1 and 32");

    UINT          qualityLevels = 0;
    const HRESULT hr            = g_renderer.pDevice->CheckMultisampleQualityLevels(_format, _sampleCount, &qualityLevels);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to query multisample quality levels. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    return qualityLevels;
}

void Renderer::CreateBuffer(const D3D11_BUFFER_DESC& _desc, const void* _pInitialData_orNull, ID3D11Buffer** _out_pBuffer)
{
    JAM_ASSERT(_out_pBuffer, "Buffer pointer is null");

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem                = _pInitialData_orNull;
    const HRESULT hr                   = g_renderer.pDevice->CreateBuffer(&_desc, &initialData, _out_pBuffer);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create buffer. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateTexture2D(const D3D11_TEXTURE2D_DESC& _desc, const void* _pInitialData_orNull, const UInt32 _initialiDataPitch_orZero, ID3D11Texture2D** _out_pTexture)
{
    JAM_ASSERT(_out_pTexture, "Texture pointer is null");

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem                = _pInitialData_orNull;
    initialData.SysMemPitch            = _initialiDataPitch_orZero;
    const HRESULT hr                   = g_renderer.pDevice->CreateTexture2D(&_desc, &initialData, _out_pTexture);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create texture 2D. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateShaderResourceView(ID3D11Resource* _pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* _pDesc, ID3D11ShaderResourceView** _out_pSRV)
{
    JAM_ASSERT(_out_pSRV, "Shader Resource View pointer is null");

    const HRESULT hr = g_renderer.pDevice->CreateShaderResourceView(_pResource, _pDesc, _out_pSRV);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create shader resource view. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateRenderTargetView(ID3D11Resource* _pResource, const D3D11_RENDER_TARGET_VIEW_DESC* _pDesc, ID3D11RenderTargetView** _out_pRTV)
{
    JAM_ASSERT(_out_pRTV, "Render Target View pointer is null");

    const HRESULT hr = g_renderer.pDevice->CreateRenderTargetView(_pResource, _pDesc, _out_pRTV);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create render target view. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateDepthStencilView(ID3D11Resource* _pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* _pDesc, ID3D11DepthStencilView** _out_pDSV)
{
    JAM_ASSERT(_out_pDSV, "Depth Stencil View pointer is null");

    const HRESULT hr = g_renderer.pDevice->CreateDepthStencilView(_pResource, _pDesc, _out_pDSV);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create depth stencil view. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* _pInputElementDescs, const UINT _numElements, ID3D11InputLayout** _out_pInputLayout)
{
    JAM_ASSERT(_out_pInputLayout, "Input Layout pointer is null");

    std::string dummyVS = "struct VSInput {";
    for (UINT i = 0; i < _numElements; ++i)
    {
        const D3D11_INPUT_ELEMENT_DESC& element = _pInputElementDescs[i];
        switch (element.Format)
        {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                dummyVS += "float4 ";
                break;
            case DXGI_FORMAT_R32G32B32_FLOAT:
                dummyVS += "float3 ";
                break;
            case DXGI_FORMAT_R32G32_FLOAT:
                dummyVS += "float2 ";
                break;
            case DXGI_FORMAT_R32_FLOAT:
                dummyVS += "float ";
                break;
            default:
                JAM_CRASH("Unsupported input element format");
        }
        dummyVS += std::format("{} : {};", element.SemanticName, element.SemanticIndex);
    }
    dummyVS += "};";
    dummyVS += R"(
        float4 main(VSInput input) : SV_POSITION
        {
            return float4(0.f,0.f,0.f,1.f);
        }
    )";

    ShaderCompiler compiler;
    if (!compiler.CompileHLSL(dummyVS, "main", "vs_5_0", nullptr, false))
    {
        JAM_CRASH("Failed to compile dummy vertex shader for input layout creation.");
    }
    const ComPtr<ID3DBlob> blob = compiler.GetCompiledBlob();
    const HRESULT          hr   = g_renderer.pDevice->CreateInputLayout(
        _pInputElementDescs,
        _numElements,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        _out_pInputLayout);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create input layout. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateVertexShader(const void* _pShaderBytecode, const SIZE_T _bytecodeLength, ID3D11VertexShader** _out_pVertexShader)
{
    JAM_ASSERT(_out_pVertexShader, "Vertex Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateVertexShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pVertexShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create vertex shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreatePixelShader(const void* _pShaderBytecode, const SIZE_T _bytecodeLength, ID3D11PixelShader** _out_pPixelShader)
{
    JAM_ASSERT(_out_pPixelShader, "Pixel Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreatePixelShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pPixelShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create pixel shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateHullShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11HullShader** _out_pHullShader)
{
    JAM_ASSERT(_out_pHullShader, "Hull Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateHullShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pHullShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create hull shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateDomainShader(const void* _pShaderBytecode, SIZE_T _bytecodeLength, ID3D11DomainShader** _out_pDomainShader)
{
    JAM_ASSERT(_out_pDomainShader, "Domain Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateDomainShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pDomainShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create domain shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateGeometryShader(const void* _pShaderBytecode, const SIZE_T _bytecodeLength, ID3D11GeometryShader** _out_pGeometryShader)
{
    JAM_ASSERT(_out_pGeometryShader, "Geometry Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateGeometryShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pGeometryShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create geometry shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateComputeShader(const void* _pShaderBytecode, const SIZE_T _bytecodeLength, ID3D11ComputeShader** _out_pComputeShader)
{
    JAM_ASSERT(_out_pComputeShader, "Compute Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateComputeShader(_pShaderBytecode, _bytecodeLength, nullptr, _out_pComputeShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create compute shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateSamplerState(const D3D11_SAMPLER_DESC& _desc, ID3D11SamplerState** _out_pSamplerState)
{
    JAM_ASSERT(_out_pSamplerState, "Sampler State pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateSamplerState(&_desc, _out_pSamplerState);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create sampler state. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateBlendState(const D3D11_BLEND_DESC& _desc, ID3D11BlendState** _out_pBlendState)
{
    JAM_ASSERT(_out_pBlendState, "Blend State pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateBlendState(&_desc, _out_pBlendState);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create blend state. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& _desc, ID3D11DepthStencilState** _out_pDepthStencilState)
{
    JAM_ASSERT(_out_pDepthStencilState, "Depth Stencil State pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateDepthStencilState(&_desc, _out_pDepthStencilState);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create depth stencil state. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateRasterizerState(const D3D11_RASTERIZER_DESC& _desc, ID3D11RasterizerState** _out_pRasterizerState)
{
    JAM_ASSERT(_out_pRasterizerState, "Rasterizer State pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateRasterizerState(&_desc, _out_pRasterizerState);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create rasterizer state. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::SetVertexBuffer(ID3D11Buffer* _pVertexBuffer, const UInt32 _stride)
{
    const UInt32   stride[] = { _stride };
    constexpr UINT offset[] = { 0 };

    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetVertexBuffers(0, 1, &_pVertexBuffer, stride, offset);
}

void Renderer::SetIndexBuffer(ID3D11Buffer* _pIndexBuffer, const bool _bUseExtendedIndex)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetIndexBuffer(_pIndexBuffer, _bUseExtendedIndex ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
}

void Renderer::SetInputLayout(ID3D11InputLayout* _pInputLayout)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetInputLayout(_pInputLayout);
}

void Renderer::SetVertexShader(ID3D11VertexShader* _pVertexShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->VSSetShader(_pVertexShader, nullptr, 0);
}

void Renderer::SetPixelShader(ID3D11PixelShader* _pPixelShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->PSSetShader(_pPixelShader, nullptr, 0);
}

void Renderer::SetGeometryShader(ID3D11GeometryShader* _pGeometryShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->GSSetShader(_pGeometryShader, nullptr, 0);
}

void Renderer::SetHullShader(ID3D11HullShader* _pHullShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->HSSetShader(_pHullShader, nullptr, 0);
}

void Renderer::SetDomainShader(ID3D11DomainShader* _pDomainShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->DSSetShader(_pDomainShader, nullptr, 0);
}

void Renderer::SetComputeShader(ID3D11ComputeShader* _pComputeShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->CSSetShader(_pComputeShader, nullptr, 0);
}

void Renderer::SetBlendState(ID3D11BlendState* _pBlendState, FLOAT _blendFactor[4])
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetBlendState(_pBlendState, _blendFactor, 0xFFFFFFFF);
}

void Renderer::SetDepthStencilState(ID3D11DepthStencilState* _pDepthStencilState, const UINT _stencilRef)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetDepthStencilState(_pDepthStencilState, _stencilRef);
}

void Renderer::SetRasterizerState(ID3D11RasterizerState* _pRasterizerState)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->RSSetState(_pRasterizerState);
}

void Renderer::SetSamplerStates(eShader _shader, const UInt32 _slot, const UInt32 _samplerCount, ID3D11SamplerState* const* _ppSamplerState)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        case eShader::PixelShader:
            ctx->PSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        case eShader::GeometryShader:
            ctx->GSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        case eShader::ComputeShader:
            ctx->CSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        case eShader::HullShader:
            ctx->HSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        case eShader::DomainShader:
            ctx->DSSetSamplers(_slot, _samplerCount, _ppSamplerState);
            break;
        default:
            JAM_ERROR("Invalid shader type for setting sampler states: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::SetShaderResourceViews(eShader _shader, const UInt32 _slot, const UInt32 _srvCount, ID3D11ShaderResourceView* const* _ppSRV)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        case eShader::PixelShader:
            ctx->PSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        case eShader::GeometryShader:
            ctx->GSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        case eShader::ComputeShader:
            ctx->CSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        case eShader::HullShader:
            ctx->HSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        case eShader::DomainShader:
            ctx->DSSetShaderResources(_slot, _srvCount, _ppSRV);
            break;
        default:
            JAM_ERROR("Invalid shader type for setting shader resource views: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::SetConstantBuffers(eShader _shader, const UInt32 _slot, const UInt32 _cbCount, ID3D11Buffer* const* _ppCB)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        case eShader::PixelShader:
            ctx->PSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        case eShader::GeometryShader:
            ctx->GSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        case eShader::ComputeShader:
            ctx->CSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        case eShader::HullShader:
            ctx->HSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        case eShader::DomainShader:
            ctx->DSSetConstantBuffers(_slot, _cbCount, _ppCB);
            break;
        default:
            JAM_ERROR("Invalid shader type for setting constant buffers: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::SetViewports(const UInt32 _numViewports, const D3D11_VIEWPORT* _pViewports)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->RSSetViewports(_numViewports, _pViewports);
}

void Renderer::SetRenderTargetViews(const UInt32 _rtvCount, ID3D11RenderTargetView* const* _ppRTV, ID3D11DepthStencilView* _pDSV)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetRenderTargets(_rtvCount, _ppRTV, _pDSV);
}

void Renderer::Draw(const UInt32 _vertexCount, const UInt32 _startVertexLocation)
{
    JAM_ASSERT(_vertexCount > 0, "Vertex count must be greater than 0");

    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->Draw(_vertexCount, _startVertexLocation);
}

void Renderer::DrawIndices(const UInt32 _indexCount, const UInt32 _startIndexLocation, const Int32 _baseVertexLocation)
{
    JAM_ASSERT(_indexCount > 0, "Index count must be greater than 0");

    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->DrawIndexed(_indexCount, _startIndexLocation, _baseVertexLocation);
}

void Renderer::OnResize_(Int32 _width, Int32 _height)
{
}

}   // namespace jam