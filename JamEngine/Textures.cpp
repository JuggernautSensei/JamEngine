#include "pch.h"

#include "Textures.h"

#include "Renderer.h"
#include "StringUtilities.h"
#include "WindowsUtilities.h"

#include <DirectXTex.h>
#include <DirectXTex.inl>
#include <DirectXTexEXR.h>

#pragma comment(lib, "DirectXTex.lib")

namespace
{

jam::UInt32 GetD3D11BindFlags(const jam::eViewFlags _viewFlags)
{
    jam::UInt32 bindFlags = 0;
    if (_viewFlags & jam::eViewFlags_ShaderResource) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (_viewFlags & jam::eViewFlags_RenderTarget) bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (_viewFlags & jam::eViewFlags_DepthStencil) bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    return bindFlags;
}

jam::eViewFlags GetJamViewFlags(const jam::UInt32 _bindFlags)
{
    jam::eViewFlags viewFlags = jam::eViewFlags_None;
    if (_bindFlags & D3D11_BIND_SHADER_RESOURCE) viewFlags |= jam::eViewFlags_ShaderResource;
    if (_bindFlags & D3D11_BIND_RENDER_TARGET) viewFlags |= jam::eViewFlags_RenderTarget;
    if (_bindFlags & D3D11_BIND_DEPTH_STENCIL) viewFlags |= jam::eViewFlags_DepthStencil;
    return viewFlags;
}

jam::UInt32 GetD3D11AccessFlags(const jam::eResourceAccess _access)
{
    switch (_access)
    {
        case jam::eResourceAccess::GPUWriteable: return 0;
        case jam::eResourceAccess::Immutable: return 0;
        case jam::eResourceAccess::CPUWriteable: return D3D11_CPU_ACCESS_WRITE;
        case jam::eResourceAccess::CPUReadable: return D3D11_CPU_ACCESS_READ;
        default: JAM_CRASH("Unknown resource access type");
    }
}

D3D11_USAGE GetD3D11Usage(const jam::eResourceAccess _access)
{
    switch (_access)
    {
        case jam::eResourceAccess::GPUWriteable: return D3D11_USAGE_DEFAULT;
        case jam::eResourceAccess::Immutable: return D3D11_USAGE_IMMUTABLE;
        case jam::eResourceAccess::CPUWriteable: return D3D11_USAGE_DYNAMIC;
        case jam::eResourceAccess::CPUReadable: return D3D11_USAGE_STAGING;
        default: JAM_CRASH("Unknown resource access type");
    }
}

jam::eResourceAccess GetJamResourceAccess(const D3D11_USAGE _usage)
{
    switch (_usage)
    {
        case D3D11_USAGE_DEFAULT: return jam::eResourceAccess::GPUWriteable;
        case D3D11_USAGE_IMMUTABLE: return jam::eResourceAccess::Immutable;
        case D3D11_USAGE_DYNAMIC: return jam::eResourceAccess::CPUWriteable;
        case D3D11_USAGE_STAGING: return jam::eResourceAccess::CPUReadable;
        default: JAM_CRASH("Unknown resource usage type");
    }
}

}   // namespace

namespace jam
{

void Texture2D::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, eResourceAccess _access, const eViewFlags _viewFlags, const UInt32 _arraySize, const UInt32 _samples, const bool _bGenerateMips, const bool _bCubemap, const std::optional<Texture2DInitializeData>& _initializeData)
{
    // reset
    *this = Texture2D();

    D3D11_TEXTURE2D_DESC desc;
    desc.Width              = _width;
    desc.Height             = _height;
    desc.MipLevels          = _bGenerateMips ? 0 : 1;   // 0 means all mips
    desc.ArraySize          = _arraySize;
    desc.Format             = _format;
    desc.SampleDesc.Count   = _samples;
    desc.SampleDesc.Quality = Renderer::GetMaxMultisampleQuality(_format, _samples);
    desc.Usage              = GetD3D11Usage(_access);
    desc.CPUAccessFlags     = GetD3D11AccessFlags(_access);
    desc.BindFlags          = GetD3D11BindFlags(_viewFlags);

    desc.MiscFlags = 0;
    if (_bCubemap) desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (_bGenerateMips) desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

    if (desc.Usage == D3D11_USAGE_IMMUTABLE)
    {
        JAM_ASSERT(_initializeData, "Initial data must be provided for immutable texture");
    }

    Renderer::CreateTexture2D(desc, _initializeData, m_texture.GetAddressOf());

    m_width      = desc.Width;
    m_height     = desc.Height;
    m_format     = desc.Format;
    m_arraySize  = desc.ArraySize;
    m_samples    = desc.SampleDesc.Count;
    m_bHasMips   = desc.MipLevels > 1;
    m_bIsCubemap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
    m_access     = GetJamResourceAccess(desc.Usage);
    m_viewFlags  = _viewFlags;
}

void Texture2D::InitializeFromSwapChain(IDXGISwapChain* _pSwapChain)
{
    JAM_ASSERT(_pSwapChain, "Swap chain pointer is null");

    // reset
    *this = Texture2D();

    HRESULT hr;
    hr = _pSwapChain->GetBuffer(0, IID_PPV_ARGS(m_texture.GetAddressOf()));
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to get texture from swap chain. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    D3D11_TEXTURE2D_DESC desc;
    m_texture->GetDesc(&desc);
    m_width      = desc.Width;
    m_height     = desc.Height;
    m_format     = desc.Format;
    m_arraySize  = desc.ArraySize;
    m_samples    = desc.SampleDesc.Count;
    m_bHasMips   = desc.MipLevels > 1;
    m_bIsCubemap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
    m_access     = static_cast<eResourceAccess>(desc.Usage);
    m_viewFlags  = GetJamViewFlags(desc.BindFlags);
}

bool Texture2D::LoadFromFile(const fs::path& _filePath, const eResourceAccess _access, const eViewFlags _viewFlags, const bool _bGenrateMips, const bool _bInverseGamma, bool _bCubeMap)
{
    // reset
    *this = Texture2D();

    DirectX::TexMetadata  metadata;
    DirectX::ScratchImage scratchImage;

    HRESULT            hr;
    const fs::path     fileExtension      = _filePath.extension();
    const std::wstring lowerFileExtension = ToLower(fileExtension.native());
    switch (HashOf(lowerFileExtension))
    {
        case L".dds"_hs:
            hr = DirectX::LoadFromDDSFile(_filePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
            break;

        case L".hdr"_hs:
            hr = DirectX::LoadFromHDRFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case L".exr"_hs:
            hr = DirectX::LoadFromEXRFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case L".tga"_hs:
            hr = DirectX::LoadFromTGAFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case L".png"_hs:
        case L".jpg"_hs:
        case L".jpeg"_hs:
        case L".bmp"_hs:
        case L".gif"_hs:
        case L".ico"_hs:
        case L".heif"_hs:
        case L".heic"_hs:
            hr = DirectX::LoadFromWICFile(_filePath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
            break;

        default:
            hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        JAM_ERROR("Unsupported texture file format: '{}'. Supported formats are: .dds, .hdr, .exr, .tga, .png, .jpg, .jpeg, .bmp, .gif, .ico, .heif, .heic", _filePath.string());
        return false;
    }

    // options
    {
        if (_bGenrateMips)
        {
            if (metadata.mipLevels == 1)
            {
                hr = DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, DirectX::TEX_FILTER_DEFAULT, 0, scratchImage);
                if (FAILED(hr))
                {
                    JAM_ERROR("Failed to generate mipmaps for texture file '{}'. HRESULT: {}", _filePath.string(), GetSystemErrorMessage(hr));
                    return false;
                }
            }
            else
            {
                metadata.mipLevels = 0;
            }
        }

        if (_bInverseGamma)
        {
            metadata.format = DirectX::MakeSRGB(metadata.format);
            if (metadata.format == DXGI_FORMAT_UNKNOWN)
            {
                JAM_ERROR("Failed to convert texture format to sRGB for file '{}'.", _filePath.string());
                return false;
            }
        }
    }

    // misc flags
    UInt32 miscFlags = 0;
    if (_bCubeMap)
    {
        miscFlags |= DirectX::TEX_MISC_TEXTURECUBE;
        if (metadata.arraySize % 6 != 0)
        {
            JAM_ERROR("Cubemap texture must have an array size that is a multiple of 6. File: '{}'", _filePath.string());
            return false;
        }
    }

    // create texture
    ComPtr<ID3D11Resource> pResource;
    hr = DirectX::CreateTextureEx(Renderer::GetDevice(), scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, D3D11_USAGE_IMMUTABLE, GetD3D11BindFlags(_viewFlags), GetD3D11AccessFlags(_access), miscFlags, DirectX::CREATETEX_DEFAULT, pResource.GetAddressOf());

    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create texture from file '{}'. HRESULT: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return false;
    }

    hr = pResource.As(&m_texture);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to cast texture resource from file '{}'. HRESULT: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return false;
    }

    m_width      = static_cast<UInt32>(metadata.width);
    m_height     = static_cast<UInt32>(metadata.height);
    m_format     = metadata.format;
    m_arraySize  = static_cast<UInt32>(metadata.arraySize);
    m_samples    = 1;
    m_bHasMips   = metadata.mipLevels > 1;
    m_bIsCubemap = (metadata.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0;
    m_access     = _access;
    m_viewFlags  = _viewFlags;
    return true;
}

void Texture2D::AttachSRV(DXGI_FORMAT _format)
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot attach SRV.");
    JAM_ASSERT(m_srv == nullptr, "Shader Resource View is already attached to this texture.");
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = _format;

    if (IsMultiSamplingTexture())
    {
        JAM_ASSERT(!IsCubemap(), "Cubemap texture cannot be multi-sampled.");

        if (IsArray())
        {
            srvDesc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srvDesc.Texture2DMSArray.ArraySize       = m_arraySize;
            srvDesc.Texture2DMSArray.FirstArraySlice = 0;
        }
        else
        {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (IsCubemap())
        {
            if (IsArray())
            {
                JAM_ASSERT((m_arraySize % 6) == 0, "Cubemap texture must have an array size of 6.");
                srvDesc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels       = m_bHasMips ? 0 : 1;   // 0 means all mips
                srvDesc.TextureCubeArray.NumCubes        = m_arraySize / 6;
            }
            else
            {
                srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels       = m_bHasMips ? 0 : 1;   // 0 means all mips
            }
        }
        else
        {
            if (IsArray())
            {
                srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels       = m_bHasMips ? 0 : 1;   // 0 means all mips
                srvDesc.Texture2DArray.ArraySize       = m_arraySize;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
            }
            else
            {
                srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels       = m_bHasMips ? 0 : 1;   // 0 means all mips
            }
        }
    }

    Renderer::CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());
}

void Texture2D::AttachRTV(const DXGI_FORMAT _format)
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot attach RTV.");
    JAM_ASSERT(m_rtv == nullptr, "Render Target View is already attached to this texture.");

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format                        = _format;

    JAM_ASSERT(!IsCubemap(), "Cubemap texture cannot be used as a render target view.");

    if (IsMultiSamplingTexture())
    {
        if (IsArray())
        {
            rtvDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.ArraySize       = m_arraySize;
            rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
        }
        else
        {
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (IsArray())
        {
            rtvDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.ArraySize       = m_arraySize;
            rtvDesc.Texture2DArray.FirstArraySlice = 0;
        }
        else
        {
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
    }

    Renderer::CreateRenderTargetView(m_texture.Get(), &rtvDesc, m_rtv.GetAddressOf());
}

void Texture2D::AttachDSV(const DXGI_FORMAT _format)
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot attach RTV.");
    JAM_ASSERT(m_dsv == nullptr, "Render Target View is already attached to this texture.");

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format                        = _format;

    JAM_ASSERT(!IsCubemap(), "Cubemap texture cannot be used as a render target view.");

    if (IsMultiSamplingTexture())
    {
        if (IsArray())
        {
            dsvDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDesc.Texture2DMSArray.ArraySize       = m_arraySize;
            dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
        }
        else
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (IsArray())
        {
            dsvDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.ArraySize       = m_arraySize;
            dsvDesc.Texture2DArray.FirstArraySlice = 0;
        }
        else
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        }
    }

    Renderer::CreateDepthStencilView(m_texture.Get(), &dsvDesc, m_dsv.GetAddressOf());
}

Int32 Texture2D::DetachSRV()
{
    JAM_ASSERT(m_srv, "Shader Resource View is not attached to this texture.");
    return m_srv.Reset();
}

Int32 Texture2D::DetachRTV()
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    return m_rtv.Reset();
}

Int32 Texture2D::DetachDSV()
{
    JAM_ASSERT(m_dsv, "Depth Stencil View is not attached to this texture.");
    return m_dsv.Reset();
}

void Texture2D::BindAsShaderResource(const eShader _shader, const UInt32 _slot) const
{
    JAM_ASSERT(m_srv, "Shader Resource View is not attached to this texture.");
    ID3D11ShaderResourceView* srvArray[] = { GetSRV() };
    Renderer::BindShaderResourceViews(_shader, _slot, srvArray);
}

void Texture2D::BindAsRenderTargetWithDepthStencil(const Texture2D& _depthStencil) const
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    JAM_ASSERT(_depthStencil.m_dsv, "Depth Stencil View is not attached to the depth stencil texture.");
    ID3D11RenderTargetView* rtvArray[] = { GetRTV() };
    Renderer::BindRenderTargetViews(rtvArray, _depthStencil.GetDSV());
}

void Texture2D::BindAsRenderTarget() const
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    ID3D11RenderTargetView* rtvArray[] = { GetRTV() };
    Renderer::BindRenderTargetViews(rtvArray, nullptr);
}

void Texture2D::ClearRenderTarget(const float _color[4]) const
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();
    dc->ClearRenderTargetView(m_rtv.Get(), _color);
}

void Texture2D::ClearDepthStencil(const bool _bClearDepth, const bool _bClearStencil, const float _depth, const UInt8 _stencil) const
{
    JAM_ASSERT(m_dsv, "Depth Stencil View is not attached to this texture.");
    ID3D11DeviceContext* dc         = Renderer::GetDeviceContext();
    UINT                 clearFlags = 0;
    if (_bClearDepth) clearFlags |= D3D11_CLEAR_DEPTH;
    if (_bClearStencil) clearFlags |= D3D11_CLEAR_STENCIL;
    dc->ClearDepthStencilView(m_dsv.Get(), clearFlags, _depth, _stencil);
}

UInt32 Texture2D::Reset()
{
    if (m_srv) m_srv.Reset();
    if (m_rtv) m_rtv.Reset();
    if (m_dsv) m_dsv.Reset();
    UInt32 refCount = m_texture.Reset();
    *this           = Texture2D();
    return refCount;
}

ID3D11ShaderResourceView* Texture2D::GetSRV() const
{
    JAM_ASSERT(m_srv, "Shader Resource View is not attached to this texture.");
    return m_srv.Get();
}

ID3D11RenderTargetView* Texture2D::GetRTV() const
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    return m_rtv.Get();
}

ID3D11DepthStencilView* Texture2D::GetDSV() const
{
    JAM_ASSERT(m_dsv, "Depth Stencil View is not attached to this texture.");
    return m_dsv.Get();
}

}   // namespace jam