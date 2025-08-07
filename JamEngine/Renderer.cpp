#include "pch.h"

#include "Renderer.h"

#include "Application.h"
#include "Buffers.h"
#include "Event.h"
#include "ShaderCompiler.h"
#include "Textures.h"
#include "Vertex.h"
#include "WindowsUtilities.h"

namespace
{

struct RendererContext
{
    jam::ComPtr<ID3D11Device>        pDevice;
    jam::ComPtr<ID3D11DeviceContext> pDeviceContext;
    jam::ComPtr<IDXGISwapChain1>     pSwapChain;
    jam::Texture2D                   backBufferTexture;

    // pipeline
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

    // for full screen quad
    jam::VertexBuffer fullScreenQuadVB;
    jam::IndexBuffer  fullScreenQuadIB;
    bool              bFullScreenQuadShaderInitialized = false;
};

RendererContext g_renderer;

}   // namespace

namespace jam
{

void Renderer::Initialize()
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
        const Window& window       = GetApplication().GetWindow();
        const HWND    hWnd         = window.GetPlatformHandle();
        const auto [width, height] = window.GetWindowSize();

        // create swap chain
        ComPtr<IDXGIDevice2> dxgiDevice;
        hr = g_renderer.pDevice->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIDevice1 from D3D11 device. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        hr = dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.GetAddressOf()));
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIAdapter1 from IDXGIDevice1. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        ComPtr<IDXGIFactory2> dxgiFactory;
        hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to get IDXGIFactory1 from IDXGIAdapter1. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width              = width;
        swapChainDesc.Height             = height;
        swapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo             = false;
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount        = 2;
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;

        hr = dxgiFactory->CreateSwapChainForHwnd(GetDevice(), hWnd, &swapChainDesc, nullptr, nullptr, g_renderer.pSwapChain.GetAddressOf());
        if (FAILED(hr))
        {
            JAM_CRASH("Failed to create swap chain. HRESULT: {}", GetSystemErrorMessage(hr));
        }

        // create back buffer texture & render target view
        g_renderer.backBufferTexture.InitializeFromSwapChain(g_renderer.pSwapChain.Get());
    }
}

void Renderer::OnEvent(const Event& _event)
{
    // do not use event dispatcher
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetName());

    if (_event.GetHash() == WindowResizeEvent::k_staticHash)
    {
        // this is allowed casting
        const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(_event);
        OnResize_(resizeEvent);
    }
}

void Renderer::Present(const bool _bVSync)
{
    JAM_ASSERT(g_renderer.pSwapChain, "Swap chain is not initialized");

    const HRESULT hr = g_renderer.pSwapChain->Present(_bVSync ? 1 : 0, 0);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to present swap chain. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

const Texture2D& Renderer::GetBackBufferTexture()
{
    return g_renderer.backBufferTexture;
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
    if (FAILED(hr) || qualityLevels == 0)
    {
        JAM_CRASH("Failed to query multisample quality levels. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    return qualityLevels - 1;
}

void Renderer::CreateBuffer(const D3D11_BUFFER_DESC& _desc, const std::optional<BufferInitializeData>& _initializeData, ID3D11Buffer** _out_pBuffer)
{
    JAM_ASSERT(_out_pBuffer, "Buffer pointer is null");

    HRESULT hr;
    if (_initializeData)
    {
        D3D11_SUBRESOURCE_DATA initialData;
        initialData.pSysMem = _initializeData.value().pData;
        hr                  = g_renderer.pDevice->CreateBuffer(&_desc, &initialData, _out_pBuffer);
    }
    else
    {
        hr = g_renderer.pDevice->CreateBuffer(&_desc, nullptr, _out_pBuffer);
    }

    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create buffer. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateTexture2D(const D3D11_TEXTURE2D_DESC& _desc, const std::optional<Texture2DInitializeData>& _initializeData, ID3D11Texture2D** _out_pTexture)
{
    JAM_ASSERT(_out_pTexture, "Texture pointer is null");

    HRESULT hr;

    if (_initializeData)
    {
        const Texture2DInitializeData& intializeData = _initializeData.value();

        D3D11_SUBRESOURCE_DATA initialData;
        initialData.pSysMem     = intializeData.pData;
        initialData.SysMemPitch = intializeData.pitch;
        hr                      = g_renderer.pDevice->CreateTexture2D(&_desc, &initialData, _out_pTexture);
    }
    else
    {
        hr = g_renderer.pDevice->CreateTexture2D(&_desc, nullptr, _out_pTexture);
    }

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

void Renderer::CreateInputLayout(const std::span<const D3D11_INPUT_ELEMENT_DESC> _inputElements, ID3DBlob* _pVertexShaderBlob, ID3D11InputLayout** _out_pInputLayout)
{
    JAM_ASSERT(_out_pInputLayout, "Input Layout pointer is null");
    JAM_ASSERT(_pVertexShaderBlob, "Vertex Shader Blob pointer is null");

    const HRESULT hr = g_renderer.pDevice->CreateInputLayout(_inputElements.data(),
                                                             static_cast<UINT>(_inputElements.size()),
                                                             _pVertexShaderBlob->GetBufferPointer(),
                                                             _pVertexShaderBlob->GetBufferSize(),
                                                             _out_pInputLayout);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create input layout. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateInputLayout(const std::span<const D3D11_INPUT_ELEMENT_DESC> _inputElements, ID3D11InputLayout** _out_pInputLayout)
{
    JAM_ASSERT(_out_pInputLayout, "Input Layout pointer is null");

    std::string dummyVS = "struct VSInput {";
    for (const D3D11_INPUT_ELEMENT_DESC& elem: _inputElements)
    {
        switch (elem.Format)
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
        dummyVS += std::format("{} : {};", elem.SemanticName, elem.SemanticIndex);
    }
    dummyVS += "};";
    dummyVS += R"(
        float4 main(VSInput input) : SV_POSITION
        {
            return float4(0.f,0.f,0.f,1.f);
        }
    )";

    ShaderCompiler compiler;
    if (!compiler.CompileHLSL(dummyVS, "main", "vs_5_0", eShaderCompileOption::Optimized))
    {
        JAM_CRASH("Failed to compile dummy vertex shader for input layout creation.");
    }
    ComPtr<ID3DBlob> blob;
    compiler.GetCompiledShader(blob.GetAddressOf());
    const HRESULT hr = g_renderer.pDevice->CreateInputLayout(_inputElements.data(), static_cast<UINT>(_inputElements.size()), blob->GetBufferPointer(), blob->GetBufferSize(), _out_pInputLayout);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create input layout. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateVertexShader(const ShaderCreationData& _data, ID3D11VertexShader** _out_pVertexShader)
{
    JAM_ASSERT(_out_pVertexShader, "Vertex Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateVertexShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pVertexShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create vertex shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreatePixelShader(const ShaderCreationData& _data, ID3D11PixelShader** _out_pPixelShader)
{
    JAM_ASSERT(_out_pPixelShader, "Pixel Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreatePixelShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pPixelShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create pixel shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateHullShader(const ShaderCreationData& _data, ID3D11HullShader** _out_pHullShader)
{
    JAM_ASSERT(_out_pHullShader, "Hull Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateHullShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pHullShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create hull shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateDomainShader(const ShaderCreationData& _data, ID3D11DomainShader** _out_pDomainShader)
{
    JAM_ASSERT(_out_pDomainShader, "Domain Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateDomainShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pDomainShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create domain shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateGeometryShader(const ShaderCreationData& _data, ID3D11GeometryShader** _out_pGeometryShader)
{
    JAM_ASSERT(_out_pGeometryShader, "Geometry Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateGeometryShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pGeometryShader);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to create geometry shader. HRESULT: {}", GetSystemErrorMessage(hr));
    }
}

void Renderer::CreateComputeShader(const ShaderCreationData& _data, ID3D11ComputeShader** _out_pComputeShader)
{
    JAM_ASSERT(_out_pComputeShader, "Compute Shader pointer is null");
    const HRESULT hr = g_renderer.pDevice->CreateComputeShader(_data.pBytecode, _data.bytecodeLength, nullptr, _out_pComputeShader);
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

void Renderer::BindTopology(const D3D11_PRIMITIVE_TOPOLOGY _topology)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();

    if (g_renderer.topology != _topology)
    {
        g_renderer.topology = _topology;
        ctx->IASetPrimitiveTopology(_topology);
    }
}

void Renderer::BindVertexBuffer(ID3D11Buffer* _pVertexBuffer, const UInt32 _stride)
{
    const UInt32   stride[] = { _stride };
    constexpr UINT offset[] = { 0 };

    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetVertexBuffers(0, 1, &_pVertexBuffer, stride, offset);
}

void Renderer::BindIndexBuffer(ID3D11Buffer* _pIndexBuffer)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::BindInputLayout(ID3D11InputLayout* _pInputLayout)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->IASetInputLayout(_pInputLayout);
}

void Renderer::BindVertexShader(ID3D11VertexShader* _pVertexShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->VSSetShader(_pVertexShader, nullptr, 0);
}

void Renderer::BindPixelShader(ID3D11PixelShader* _pPixelShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->PSSetShader(_pPixelShader, nullptr, 0);
}

void Renderer::BindGeometryShader(ID3D11GeometryShader* _pGeometryShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->GSSetShader(_pGeometryShader, nullptr, 0);
}

void Renderer::BindHullShader(ID3D11HullShader* _pHullShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->HSSetShader(_pHullShader, nullptr, 0);
}

void Renderer::BindDomainShader(ID3D11DomainShader* _pDomainShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->DSSetShader(_pDomainShader, nullptr, 0);
}

void Renderer::BindComputeShader(ID3D11ComputeShader* _pComputeShader)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->CSSetShader(_pComputeShader, nullptr, 0);
}

void Renderer::BindBlendState(ID3D11BlendState* _pBlendState, const FLOAT _blendFactor[4])
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetBlendState(_pBlendState, _blendFactor, 0xFFFFFFFF);
}

void Renderer::BindDepthStencilState(ID3D11DepthStencilState* _pDepthStencilState, const UINT _stencilRef)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetDepthStencilState(_pDepthStencilState, _stencilRef);
}

void Renderer::BindRasterizerState(ID3D11RasterizerState* _pRasterizerState)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->RSSetState(_pRasterizerState);
}

void Renderer::BindSamplerStates(eShader _shader, const UInt32 _slot, const std::span<ID3D11SamplerState* const> _samplers)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        case eShader::PixelShader:
            ctx->PSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        case eShader::GeometryShader:
            ctx->GSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        case eShader::ComputeShader:
            ctx->CSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        case eShader::HullShader:
            ctx->HSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        case eShader::DomainShader:
            ctx->DSSetSamplers(_slot, static_cast<UINT>(_samplers.size()), _samplers.data());
            break;
        default:
            JAM_ERROR("Invalid shader type for setting sampler states: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::BindShaderResourceViews(eShader _shader, const UInt32 _slot, const std::span<ID3D11ShaderResourceView* const> _resources)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        case eShader::PixelShader:
            ctx->PSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        case eShader::GeometryShader:
            ctx->GSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        case eShader::ComputeShader:
            ctx->CSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        case eShader::HullShader:
            ctx->HSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        case eShader::DomainShader:
            ctx->DSSetShaderResources(_slot, static_cast<UINT>(_resources.size()), _resources.data());
            break;
        default:
            JAM_ERROR("Invalid shader type for setting shader resource views: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::BindConstantBuffers(eShader _shader, const UInt32 _slot, const std::span<ID3D11Buffer* const> _buffers)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    switch (_shader)
    {
        case eShader::VertexShader:
            ctx->VSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        case eShader::PixelShader:
            ctx->PSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        case eShader::GeometryShader:
            ctx->GSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        case eShader::ComputeShader:
            ctx->CSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        case eShader::HullShader:
            ctx->HSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        case eShader::DomainShader:
            ctx->DSSetConstantBuffers(_slot, static_cast<UINT>(_buffers.size()), _buffers.data());
            break;
        default:
            JAM_ERROR("Invalid shader type for setting constant buffers: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::BindViewports(const std::span<const D3D11_VIEWPORT> _viewports)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->RSSetViewports(static_cast<UINT>(_viewports.size()), _viewports.data());
}

void Renderer::BindRenderTargetViews(const std::span<ID3D11RenderTargetView* const> _renderTargets, ID3D11DepthStencilView* _pDSV)
{
    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetRenderTargets(static_cast<UINT>(_renderTargets.size()), _renderTargets.data(), _pDSV);
}

void Renderer::UnbindSamplerStates(eShader _shader, const UInt32 _slot, const UInt32 _count)
{
    constexpr ID3D11SamplerState* k_nullSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};

    switch (_shader)
    {
        case eShader::VertexShader:
            g_renderer.pDeviceContext->VSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        case eShader::PixelShader:
            g_renderer.pDeviceContext->PSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        case eShader::GeometryShader:
            g_renderer.pDeviceContext->GSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        case eShader::ComputeShader:
            g_renderer.pDeviceContext->CSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        case eShader::HullShader:
            g_renderer.pDeviceContext->HSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        case eShader::DomainShader:
            g_renderer.pDeviceContext->DSSetSamplers(_slot, _count, k_nullSamplers);
            break;

        default:
            JAM_ERROR("Invalid shader type for unbinding sampler states: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::UnbindShaderResourceViews(eShader _shader, const UInt32 _slot, const UInt32 _count)
{
    constexpr ID3D11ShaderResourceView* k_nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

    switch (_shader)
    {
        case eShader::VertexShader:
            g_renderer.pDeviceContext->VSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        case eShader::PixelShader:
            g_renderer.pDeviceContext->PSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        case eShader::GeometryShader:
            g_renderer.pDeviceContext->GSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        case eShader::ComputeShader:
            g_renderer.pDeviceContext->CSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        case eShader::HullShader:
            g_renderer.pDeviceContext->HSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        case eShader::DomainShader:
            g_renderer.pDeviceContext->DSSetShaderResources(_slot, _count, k_nullSRVs);
            break;

        default:
            JAM_ERROR("Invalid shader type for unbinding shader resource views: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::UnbindConstantBuffers(eShader _shader, const UInt32 _slot, const UInt32 _count)
{
    constexpr ID3D11Buffer* k_nullBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};

    switch (_shader)
    {
        case eShader::VertexShader:
            g_renderer.pDeviceContext->VSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        case eShader::PixelShader:
            g_renderer.pDeviceContext->PSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        case eShader::GeometryShader:
            g_renderer.pDeviceContext->GSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        case eShader::ComputeShader:
            g_renderer.pDeviceContext->CSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        case eShader::HullShader:
            g_renderer.pDeviceContext->HSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        case eShader::DomainShader:
            g_renderer.pDeviceContext->DSSetConstantBuffers(_slot, _count, k_nullBuffers);
            break;

        default:
            JAM_ERROR("Invalid shader type for unbinding constant buffers: {}", static_cast<int>(_shader));
            break;
    }
}

void Renderer::UnbindRenderTargetViews()
{
    constexpr ID3D11RenderTargetView* k_nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

    ID3D11DeviceContext* ctx = g_renderer.pDeviceContext.Get();
    ctx->OMSetRenderTargets(std::size(k_nullRTVs), k_nullRTVs, nullptr);
}

void Renderer::DrawFullScreenQuad()
{
    if (g_renderer.bFullScreenQuadShaderInitialized == false)
    {
        // initialize vertex buffer
        {
            Vertex2 vertices[] = {
                Vertex2 {  Vec2 { -1.f, 1.f }, Vec2 { 0.f, 0.f } },
                Vertex2 {   Vec2 { 1.f, 1.f }, Vec2 { 1.f, 0.f } },
                Vertex2 {  Vec2 { 1.f, -1.f }, Vec2 { 1.f, 1.f } },
                Vertex2 { Vec2 { -1.f, -1.f }, Vec2 { 0.f, 1.f } },
            };

            BufferInitializeData initData;
            initData.pData = vertices;
            g_renderer.fullScreenQuadVB.Initialize(GetVertexStride(eVertexType::Vertex2), std::size(vertices), eResourceAccess::Immutable, initData);
        }

        // initialize index buffer
        {
            Index indices[] = { 0, 1, 2, 0, 2, 3 };

            IndexBufferInitializeData initData;
            initData.pData = indices;
            g_renderer.fullScreenQuadIB.Initialize(std::size(indices), eResourceAccess::Immutable, initData);
        }

        g_renderer.bFullScreenQuadShaderInitialized = true;
    }

    BindTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_renderer.fullScreenQuadVB.Bind();
    g_renderer.fullScreenQuadIB.Bind();
    DrawIndices(6, 0, 0);
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

void Renderer::OnResize_(const WindowResizeEvent& _event)
{
    if (g_renderer.pSwapChain == nullptr)
    {
        // Renderer is not initialized. Cannot resize swap chain. This is not error, just a case when the application is starting.
        return;
    }

    // 리사이즈 전 스왑체인에 의존하는 리소스들을 먼저 해제해야 함
    {
        // 백버퍼는 특별하게 관리되어야 하기 때문에 직접 사용하는 것은 위험함
        // 직접 사용한다면 해당 이벤트를 받았을 때 사용자가 직접 스왑체인에 의존하는 리소스를 해제해야 함
        Application&           app = GetApplication();
        BackBufferCleanupEvent event;
        app.DispatchEvent(event);

        // 백버퍼 텍스처를 해제
        UInt32 refCount = g_renderer.backBufferTexture.Reset();
        JAM_ASSERT(refCount == 0, "Back buffer texture still has references. RefCount: {}", refCount);
    }

    // create or resize swap chain
    const UInt32 width  = _event.GetWidth();
    const UInt32 height = _event.GetHeight();
    HRESULT      hr     = g_renderer.pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to resize swap chain buffers. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    // create back buffer texture
    g_renderer.backBufferTexture.InitializeFromSwapChain(g_renderer.pSwapChain.Get());
}

}   // namespace jam