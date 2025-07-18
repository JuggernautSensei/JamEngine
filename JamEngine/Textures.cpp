#include "pch.h"

#include "Textures.h"

#include "StringUtilities.h"
#include "WindowsUtilities.h"

#include <DirectXTex.h>
#include <DirectXTex.inl>
#include <DirectXTexEXR.h>

#pragma comment(lib, "DirectXTex.lib")

namespace jam
{

Texture2D Texture2D::Create(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const UInt32 _arraySize, const UInt32 _samples, const bool _generateMips, const bool _cubemap, eResourceAccess _access, const eViewFlags _viewFlags, const InitializeTextureData* _pInitialData_orNull)
{
    constexpr D3D11_USAGE k_usageTable[] = {
        D3D11_USAGE_IMMUTABLE,   // Immutable
        D3D11_USAGE_DEFAULT,     // GPUWriteable
        D3D11_USAGE_STAGING,     // CPUReadable
        D3D11_USAGE_DYNAMIC,     // CPUWriteable
    };

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width                = _width;
    desc.Height               = _height;
    desc.MipLevels            = _generateMips ? 0 : 1;   // 0 means all mips
    desc.ArraySize            = _arraySize;
    desc.Format               = _format;
    desc.SampleDesc.Count     = _samples;
    desc.SampleDesc.Quality   = Renderer::GetMaxMultisampleQuality(_format, _samples);
    desc.Usage                = k_usageTable[static_cast<int>(_access)];
    desc.CPUAccessFlags       = 0;
    desc.BindFlags            = 0;
    if (_viewFlags & eViewFlags_ShaderResource) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (_viewFlags & eViewFlags_RenderTarget) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (_viewFlags & eViewFlags_DepthStencil) desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

    desc.MiscFlags = 0;
    if (_cubemap) desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (_generateMips) desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

    if (desc.Usage == D3D11_USAGE_IMMUTABLE)
    {
        JAM_ASSERT(_pInitialData_orNull, "Initial data must be provided for immutable texture");
    }

    const void*  pInitialData     = _pInitialData_orNull ? _pInitialData_orNull->pInitialData : nullptr;
    const UInt32 initialDataPitch = _pInitialData_orNull ? _pInitialData_orNull->pitch : 0;
    Texture2D    texture;
    Renderer::CreateTexture2D(desc, pInitialData, initialDataPitch, texture.m_texture.GetAddressOf());
    return texture;
}

std::optional<Texture2D> Texture2D::CreateFromFile(const fs::path& _filePath, const eViewFlags _viewFlags, bool _bGenrateMips, bool _bInverseGamma)
{
    DirectX::TexMetadata  metadata;
    DirectX::ScratchImage scratchImage;

    HRESULT            hr;
    const fs::path     fileExtension    = _filePath.extension();
    const std::wstring rawFileExtension = ToLower(fileExtension.native());
    switch (HashOf(rawFileExtension))
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
            // metadata.format = DirectX::MakeSRGB(metadata.format);

            if (metadata.format == DXGI_FORMAT_UNKNOWN)
            {
                JAM_ERROR("Failed to convert texture format to sRGB for file '{}'.", _filePath.string());
                return std::nullopt;
            }
        }
    }

    ComPtr<ID3D11Resource> pResource;
    hr = DirectX::CreateTexture(Renderer::GetDevice(), scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, pResource.GetAddressOf());
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
    texture.m_bGPUWrite  = true;    // GPU writeable by default
    texture.m_bCPURead   = false;   // not a staging texture
    texture.m_bCPUWrite  = false;   // not a staging texture
    texture.m_bindFlags  = _viewFlags;
    return texture;
}

void Texture2D::AttachSRV(DXGI_FORMAT _format)
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot attach SRV.");
    JAM_ASSERT(m_bindFlags & eViewFlags_ShaderResource, "Shader Resource View is not enabled for this texture.");
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
    JAM_ASSERT(m_bindFlags & eViewFlags_RenderTarget, "Render Target View is not enabled for this texture.");
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
    JAM_ASSERT(m_bindFlags & eViewFlags_RenderTarget, "Render Target View is not enabled for this texture.");
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

void Texture2D::BindAsShaderResource(const eShader _shader, const UInt32 _slot) const
{
    JAM_ASSERT(m_srv, "Shader Resource View is not attached to this texture.");
    ID3D11ShaderResourceView* srvArray[] = { m_srv.Get() };
    Renderer::SetShaderResourceViews(_shader, _slot, 1, srvArray);
}

void Texture2D::BindAsOutputResource(const bool _bBindRenderTarget, const bool _bBindDepthStencil) const
{
    if (_bBindDepthStencil)
    {
        JAM_ASSERT(m_dsv, "Depth Stencil View is not attached to this texture.");
    }

    if (_bBindRenderTarget)
    {
        JAM_ASSERT(m_rtv, "Render Target View is not attached to this texture.");
    }

    const UInt32            numViews   = _bBindRenderTarget ? 1 : 0;
    ID3D11RenderTargetView* rtvArray[] = { _bBindRenderTarget ? m_rtv.Get() : nullptr };
    ID3D11DepthStencilView* dsvArray   = _bBindDepthStencil ? m_dsv.Get() : nullptr;
    Renderer::SetRenderTargetViews(numViews, rtvArray, dsvArray);
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