#include "pch.h"

#include "Textures.h"

#include "Renderer.h"
#include "StringUtilities.h"
#include "WindowsUtilities.h"

#include <DirectXTex.h>
#include <DirectXTex.inl>
#include <DirectXTexEXR.h>

#pragma comment(lib, "DirectXTex.lib")

namespace jam
{

Texture2D Texture2D::Create(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const UInt32 _arraySize, const UInt32 _samples, const bool _bGenerateMips, const bool _bCubemap, eResourceAccess _access, const eViewFlags _viewFlags, const InitializeTextureData* _pInitialData_orNull)
{
    constexpr UINT k_accessFlagsTable[] = {
        0,                        // Immutable
        0,                        // GPUWriteable
        D3D11_CPU_ACCESS_READ,    // CPUReadable
        D3D11_CPU_ACCESS_WRITE,   // CPUWriteable
    };

    D3D11_TEXTURE2D_DESC desc;
    desc.Width              = _width;
    desc.Height             = _height;
    desc.MipLevels          = _bGenerateMips ? 0 : 1;   // 0 means all mips
    desc.ArraySize          = _arraySize;
    desc.Format             = _format;
    desc.SampleDesc.Count   = _samples;
    desc.SampleDesc.Quality = Renderer::GetMaxMultisampleQuality(_format, _samples);
    desc.Usage              = static_cast<D3D11_USAGE>(_access);
    desc.CPUAccessFlags     = k_accessFlagsTable[static_cast<int>(_access)];
    desc.BindFlags          = 0;
    if (_viewFlags & eViewFlags_ShaderResource) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (_viewFlags & eViewFlags_RenderTarget) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (_viewFlags & eViewFlags_DepthStencil) desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

    desc.MiscFlags = 0;
    if (_bCubemap) desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (_bGenerateMips) desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

    if (desc.Usage == D3D11_USAGE_IMMUTABLE)
    {
        JAM_ASSERT(_pInitialData_orNull, "Initial data must be provided for immutable texture");
    }

    const void*  pInitialData     = _pInitialData_orNull ? _pInitialData_orNull->pInitialData : nullptr;
    const UInt32 initialDataPitch = _pInitialData_orNull ? _pInitialData_orNull->pitch : 0;

    Texture2D texture;
    Renderer::CreateTexture2D(desc, pInitialData, initialDataPitch, texture.m_texture.GetAddressOf());
    texture.m_width      = desc.Width;
    texture.m_height     = desc.Height;
    texture.m_format     = desc.Format;
    texture.m_arraySize  = desc.ArraySize;
    texture.m_samples    = desc.SampleDesc.Count;
    texture.m_bHasMips   = desc.MipLevels > 1;
    texture.m_bIsCubemap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
    texture.m_access     = static_cast<eResourceAccess>(desc.Usage);
    texture.m_viewFlags  = _viewFlags;
    return texture;
}

Texture2D Texture2D::CreateFromSwapChain(IDXGISwapChain* _pSwapChain)
{
    JAM_ASSERT(_pSwapChain, "Swap chain pointer is null");

    Texture2D texture;

    HRESULT hr;
    hr = _pSwapChain->GetBuffer(0, IID_PPV_ARGS(&texture.m_texture));
    if (FAILED(hr))
    {
        JAM_CRASH("Failed to get texture from swap chain. HRESULT: {}", GetSystemErrorMessage(hr));
    }

    D3D11_TEXTURE2D_DESC desc;
    texture.m_texture->GetDesc(&desc);
    texture.m_width      = desc.Width;
    texture.m_height     = desc.Height;
    texture.m_format     = desc.Format;
    texture.m_arraySize  = desc.ArraySize;
    texture.m_samples    = desc.SampleDesc.Count;
    texture.m_bHasMips   = desc.MipLevels > 1;
    texture.m_bIsCubemap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
    texture.m_access     = static_cast<eResourceAccess>(desc.Usage);
    texture.m_viewFlags  = eViewFlags_None;
    if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) texture.m_viewFlags |= eViewFlags_ShaderResource;
    if (desc.BindFlags & D3D11_BIND_RENDER_TARGET) texture.m_viewFlags |= eViewFlags_RenderTarget;
    if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) texture.m_viewFlags |= eViewFlags_DepthStencil;
    return texture;
}

std::optional<Texture2D> Texture2D::CreateFromFile(const fs::path& _filePath, const eViewFlags _viewFlags, const bool _bGenrateMips, const bool _bInverseGamma)
{
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
        return std::nullopt;
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
                    return std::nullopt;
                }
            }
            else
            {
                metadata.mipLevels = 0;
            }
        }

        if (_bInverseGamma)
        {
            // maybe unused, but kept for reference

            // UInt8*       pPixel     = scratchImage.GetPixels();
            // const size_t pixelCount = scratchImage.GetPixelsSize();
            //
            // for (size_t i = 0; i < pixelCount; i += 4)
            // {
            //     float r = pPixel[i + 0] / 255.0f;
            //     float g = pPixel[i + 1] / 255.0f;
            //     float b = pPixel[i + 2] / 255.0f;
            //
            //     r = powf(r, 2.2f);
            //     g = powf(g, 2.2f);
            //     b = powf(b, 2.2f);
            //
            //     pPixel[i + 0] = static_cast<UInt8>(r * 255);
            //     pPixel[i + 1] = static_cast<UInt8>(g * 255);
            //     pPixel[i + 2] = static_cast<UInt8>(b * 255);
            // }
            //

            metadata.format = DirectX::MakeSRGB(metadata.format);
            if (metadata.format == DXGI_FORMAT_UNKNOWN)
            {
                JAM_ERROR("Failed to convert texture format to sRGB for file '{}'.", _filePath.string());
                return std::nullopt;
            }
        }
    }

    // bind flags
    UInt32 bindFlags = 0;
    if (_viewFlags & eViewFlags_ShaderResource) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (_viewFlags & eViewFlags_RenderTarget) bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (_viewFlags & eViewFlags_DepthStencil) bindFlags |= D3D11_BIND_DEPTH_STENCIL;

    // create texture
    ComPtr<ID3D11Resource> pResource;
    hr = DirectX::CreateTextureEx(Renderer::GetDevice(), scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, D3D11_USAGE_IMMUTABLE, bindFlags, NULL, NULL, DirectX::CREATETEX_DEFAULT, pResource.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create texture from file '{}'. HRESULT: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    Texture2D texture;
    hr = pResource.As(&texture.m_texture);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to cast texture resource from file '{}'. HRESULT: {}", _filePath.string(), GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    texture.m_width      = static_cast<UInt32>(metadata.width);
    texture.m_height     = static_cast<UInt32>(metadata.height);
    texture.m_format     = metadata.format;
    texture.m_arraySize  = static_cast<UInt32>(metadata.arraySize);
    texture.m_samples    = 1;
    texture.m_bHasMips   = metadata.mipLevels > 1;
    texture.m_bIsCubemap = (metadata.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0;
    texture.m_access     = eResourceAccess::Immutable;
    texture.m_viewFlags  = _viewFlags;
    return texture;
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
    ID3D11ShaderResourceView* srvArray[] = { m_srv.Get() };
    Renderer::SetShaderResourceViews(_shader, _slot, 1, srvArray);
}

void Texture2D::BindAsRenderTarget(const Texture2D* _depthStencilTexture_orNull) const
{
    JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    ID3D11RenderTargetView* rtvArray[] = { m_rtv.Get() };
    ID3D11DepthStencilView* dsvArray   = _depthStencilTexture_orNull ? _depthStencilTexture_orNull->GetDSV() : nullptr;
    Renderer::SetRenderTargetViews(1, rtvArray, dsvArray);
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