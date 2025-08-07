#include "pch.h"

#include "Textures.h"

#include "ImageUtilities.h"
#include "Renderer.h"
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

DirectX::WICCodecs GetWICCodecsFromImageFormat(const jam::eImageFormat _format)
{
    switch (_format)
    {
        case jam::eImageFormat::PNG: return DirectX::WIC_CODEC_PNG;
        case jam::eImageFormat::JPEG: return DirectX::WIC_CODEC_JPEG;
        case jam::eImageFormat::BMP: return DirectX::WIC_CODEC_BMP;
        case jam::eImageFormat::GIF: return DirectX::WIC_CODEC_GIF;
        case jam::eImageFormat::ICO: return DirectX::WIC_CODEC_ICO;
        case jam::eImageFormat::HEIF: return DirectX::WIC_CODEC_HEIF;
        case jam::eImageFormat::HEIC: return DirectX::WIC_CODEC_HEIF;
        default: JAM_CRASH("Unsupported WIC image format");
    }
}

}   // namespace

namespace jam
{

void Texture2D::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const eResourceAccess _access, const eViewFlags _viewFlags, const UInt32 _arraySize, const UInt32 _samples, const bool _bGenerateMips, const bool _bCubemap, const std::optional<Texture2DInitializeData>& _initializeData)
{
    // reset
    Reset();

    // validate parameters
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
    desc.MiscFlags          = 0;
    if (_bCubemap) desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (_bGenerateMips) desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

    if (desc.Usage == D3D11_USAGE_IMMUTABLE)
    {
        JAM_ASSERT(_initializeData, "Initial data must be provided for immutable texture");
    }

    // create texture
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
    Reset();

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

bool Texture2D::InitializeFromImage_(DirectX::ScratchImage&& _scratchImage, DirectX::TexMetadata&& _metadata, const eResourceAccess _access, const eViewFlags _viewFlags, const bool _bGenerateMips, const bool _bInverseGamma, const bool _bCubemap)
{
    HRESULT hr;

    // options
    if (_bGenerateMips && _metadata.mipLevels == 1)
    {
        DirectX::ScratchImage mipChain;
        hr = DirectX::GenerateMipMaps(_scratchImage.GetImages(), _scratchImage.GetImageCount(), _metadata, DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
        if (FAILED(hr))
        {
            JAM_ERROR("Failed to generate mipmaps for texture. HRESULT: {}", GetSystemErrorMessage(hr));
            return false;
        }
        _scratchImage = std::move(mipChain);
        _metadata     = _scratchImage.GetMetadata();
    }

    if (_bInverseGamma)
    {
        _metadata.format = DirectX::MakeSRGB(_metadata.format);
        if (_metadata.format == DXGI_FORMAT_UNKNOWN)
        {
            JAM_ERROR("Failed to convert texture format to sRGB");
            return false;
        }
    }

    // misc flags
    UInt32 miscFlags = 0;
    if (_bCubemap)
    {
        miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        if (_metadata.arraySize % 6 != 0)
        {
            JAM_ERROR("Cubemap texture must have an array size that is a multiple of 6.");
            return false;
        }
    }

    // create texture
    ComPtr<ID3D11Resource> pResource;
    hr = DirectX::CreateTextureEx(Renderer::GetDevice(), _scratchImage.GetImages(), _scratchImage.GetImageCount(), _metadata, GetD3D11Usage(_access), GetD3D11BindFlags(_viewFlags), GetD3D11AccessFlags(_access), miscFlags, DirectX::CREATETEX_DEFAULT, pResource.GetAddressOf());

    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create texture. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    hr = pResource.As(&m_texture);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to cast texture resource. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    m_width      = static_cast<UInt32>(_metadata.width);
    m_height     = static_cast<UInt32>(_metadata.height);
    m_format     = _metadata.format;
    m_arraySize  = static_cast<UInt32>(_metadata.arraySize);
    m_samples    = 1;
    m_bHasMips   = _metadata.mipLevels > 1;
    m_bIsCubemap = (_metadata.miscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
    m_access     = _access;
    m_viewFlags  = _viewFlags;
    return true;
}

bool Texture2D::LoadFromFile(const fs::path& _filePath, const eResourceAccess _access, const eViewFlags _viewFlags, const bool _bGenrateMips, const bool _bInverseGamma, const bool _bCubeMap)
{
    // reset
    Reset();

    DirectX::TexMetadata  metadata;
    DirectX::ScratchImage scratchImage;
    HRESULT               hr;

    eImageFormat format = GetImageFormatFromPath(_filePath);
    switch (format)
    {
        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(_filePath.c_str(), &metadata, scratchImage);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(_filePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
            break;

        default:
            if (IsWICFormat(format))
            {
                hr = DirectX::LoadFromWICFile(_filePath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
            }
            else
            {
                hr = E_FAIL;
            }
    }

    if (FAILED(hr))
    {
        JAM_ERROR("Unsupported texture file format: '{}'. Supported formats are: .dds, .hdr, .exr, .tga, .png, .jpg, .jpeg, .bmp, .gif, .ico, .heif, .heic", _filePath.string());
        return false;
    }

    return InitializeFromImage_(std::move(scratchImage), std::move(metadata), _access, _viewFlags, _bGenrateMips, _bInverseGamma, _bCubeMap);
}

bool Texture2D::LoadFromMemory(const UInt8* _pData, const size_t _dataSize, const eImageFormat _imageFormat, const eResourceAccess _access, const eViewFlags _viewFlags, const bool _bGenerateMips, const bool _bInverseGamma, const bool _bCubemap)
{
    JAM_ASSERT(_pData, "Texture2D::LoadFromMemory: Data pointer is null.");

    // reset
    Reset();

    DirectX::ScratchImage scratchImage;
    DirectX::TexMetadata  metadata;
    HRESULT               hr;

    switch (_imageFormat)
    {
        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRMemory(_pData, _dataSize, &metadata, scratchImage);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAMemory(_pData, _dataSize, &metadata, scratchImage);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSMemory(_pData, _dataSize, DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
            break;

        default:
            if (IsWICFormat(_imageFormat))
            {
                hr = DirectX::LoadFromWICMemory(_pData, _dataSize, DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
            }
            else
            {
                hr = E_FAIL;
            }
    }

    if (FAILED(hr))
    {
        JAM_ERROR("Failed to load texture from memory. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    return InitializeFromImage_(std::move(scratchImage), std::move(metadata), _access, _viewFlags, _bGenerateMips, _bInverseGamma, _bCubemap);
}

bool Texture2D::SaveToFile(const fs::path& _filePath) const
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot save to file.");

    DirectX::ScratchImage scratchImage;
    HRESULT               hr;

    hr = DirectX::CaptureTexture(Renderer::GetDevice(), Renderer::GetDeviceContext(), m_texture.Get(), scratchImage);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to capture texture for saving. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    eImageFormat imageFormat = GetImageFormatFromPath(_filePath);
    switch (imageFormat)
    {
        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*scratchImage.GetImages(), _filePath.c_str());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*scratchImage.GetImages(), _filePath.c_str());
            break;

        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*scratchImage.GetImages(), _filePath.c_str());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, _filePath.c_str());
            break;

        default:
            if (IsWICFormat(imageFormat))
            {
                hr = DirectX::SaveToWICFile(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(GetWICCodecsFromImageFormat(imageFormat)), _filePath.c_str());
            }
            else
            {
                hr = E_FAIL;
            }
    }

    if (FAILED(hr))
    {
        JAM_ERROR("Unsupported texture file format: '{}'. Supported formats are: .dds, .hdr, .exr, .tga, .png, .jpg, .jpeg, .bmp, .gif, .ico, .heif, .heic", _filePath.string());
        return false;
    }

    return true;
}

std::optional<std::vector<UInt8>> Texture2D::SaveToMemory(const eImageFormat _imageFormat) const
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot save to memory.");

    if (_imageFormat == eImageFormat::Unknown)
    {
        JAM_ERROR("Texture2D::SaveToMemory: Image format is unknown. Please specify a valid image format.");
        return std::nullopt;
    }

    DirectX::ScratchImage scratchImage;
    HRESULT               hr = DirectX::CaptureTexture(Renderer::GetDevice(), Renderer::GetDeviceContext(), m_texture.Get(), scratchImage);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to capture texture for saving. HRESULT: {}", GetSystemErrorMessage(hr));
        return std::nullopt;
    }

    DirectX::Blob blob;
    switch (_imageFormat)
    {
        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRMemory(*scratchImage.GetImages(), blob);
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAMemory(*scratchImage.GetImages(), blob);
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSMemory(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
            break;

        default:
            if (IsWICFormat(_imageFormat))
            {
                hr = DirectX::SaveToWICMemory(scratchImage.GetImages(), scratchImage.GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(GetWICCodecsFromImageFormat(_imageFormat)), blob);
            }
            else
            {
                hr = E_FAIL;
            }
    }

    if (FAILED(hr))
    {
        JAM_ERROR("Unsupported texture image format: '{}'. Supported formats are: .dds, .hdr, .exr, .tga, .png, .jpg, .jpeg, .bmp, .gif, .ico, .heif, .heic", EnumToString(_imageFormat));
        return std::nullopt;
    }

    std::vector<UInt8> data(blob.GetBufferSize());
    std::memcpy(data.data(), blob.GetBufferPointer(), blob.GetBufferSize());
    return data;
}

void Texture2D::CopyFrom(const Texture2D& _other) const
{
    JAM_ASSERT(m_texture, "Texture2D is not initialized. Cannot copy from another texture.");
    JAM_ASSERT(_other.m_texture, "Source texture is not initialized. Cannot copy from it.");

    ID3D11DeviceContext* pContext = Renderer::GetDeviceContext();
    pContext->CopyResource(m_texture.Get(), _other.m_texture.Get());
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