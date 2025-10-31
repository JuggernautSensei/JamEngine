#include "pch.h"

#include "ImageImporter.h"

#include "WindowsUtilities.h"

#include <DirectXTexEXR.h>

namespace jam
{

constexpr DXGI_FORMAT k_dxgiFormatTable[EnumCount<eFormat>()] = {
    DXGI_FORMAT_UNKNOWN,                // Unknown
    DXGI_FORMAT_R8_UNORM,               // R8_UNorm
    DXGI_FORMAT_R8_SNORM,               // R8_SNorm
    DXGI_FORMAT_R8_UINT,                // R8_UInt
    DXGI_FORMAT_R8_SINT,                // R8_Int
    DXGI_FORMAT_R16_UNORM,              // R16_UNorm
    DXGI_FORMAT_R16_SNORM,              // R16_SNorm
    DXGI_FORMAT_R16_UINT,               // R16_UInt
    DXGI_FORMAT_R16_SINT,               // R16_Int
    DXGI_FORMAT_R16_FLOAT,              // R16_Float
    DXGI_FORMAT_R32_UINT,               // R32_UInt
    DXGI_FORMAT_R32_SINT,               // R32_Int
    DXGI_FORMAT_R32_FLOAT,              // R32_Float
    DXGI_FORMAT_R8G8_UNORM,             // RG8_UNorm
    DXGI_FORMAT_R8G8_SNORM,             // RG8_SNorm
    DXGI_FORMAT_R8G8_UINT,              // RG8_UInt
    DXGI_FORMAT_R8G8_SINT,              // RG8_Int
    DXGI_FORMAT_R16G16_UNORM,           // RG16_UNorm
    DXGI_FORMAT_R16G16_SNORM,           // RG16_SNorm
    DXGI_FORMAT_R16G16_UINT,            // RG16_UInt
    DXGI_FORMAT_R16G16_SINT,            // RG16_Int
    DXGI_FORMAT_R16G16_FLOAT,           // RG16_Float
    DXGI_FORMAT_R32G32_UINT,            // RG32_UInt
    DXGI_FORMAT_R32G32_SINT,            // RG32_Int
    DXGI_FORMAT_R32G32_FLOAT,           // RG32_Float
    DXGI_FORMAT_R8G8B8A8_UNORM,         // RGBA8_UNorm
    DXGI_FORMAT_R8G8B8A8_SNORM,         // RGBA8_SNorm
    DXGI_FORMAT_R8G8B8A8_UINT,          // RGBA8_UInt
    DXGI_FORMAT_R8G8B8A8_SINT,          // RGBA8_Int
    DXGI_FORMAT_R16G16B16A16_UNORM,     // RGBA16_UNorm
    DXGI_FORMAT_R16G16B16A16_SNORM,     // RGBA16_SNorm
    DXGI_FORMAT_R16G16B16A16_UINT,      // RGBA16_UInt
    DXGI_FORMAT_R16G16B16A16_SINT,      // RGBA16_Int
    DXGI_FORMAT_R16G16B16A16_FLOAT,     // RGBA16_Float
    DXGI_FORMAT_R32G32B32A32_UINT,      // RGBA32_UInt
    DXGI_FORMAT_R32G32B32A32_SINT,      // RGBA32_Int
    DXGI_FORMAT_R32G32B32A32_FLOAT,     // RGBA32_Float
    DXGI_FORMAT_D16_UNORM,              // D16_UNorm
    DXGI_FORMAT_D24_UNORM_S8_UINT,      // D24_Unorm_S8_UInt
    DXGI_FORMAT_D32_FLOAT,              // D32_Float
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   // D32_Float_S8_UInt
};

eErrorCode ImageImporter::Import()
{
    JAM_ASSERT(!m_filePath.empty(), "file path is empty");
    JAM_ASSERT(m_format != eImageFormat::Unknown, "unknown image format");

    m_scratchImg.Release();
    std::wstring wPath = m_filePath.wstring();

    HRESULT hr = E_FAIL;
    switch (m_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(wPath.c_str(), nullptr, m_scratchImg);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, m_scratchImg);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(wPath.c_str(), nullptr, m_scratchImg);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(wPath.c_str(), nullptr, m_scratchImg);
            break;

        case eImageFormat::BMP:
        case eImageFormat::JPG:
        case eImageFormat::JPEG:
        case eImageFormat::PNG:
        case eImageFormat::TIF:
        case eImageFormat::TIFF:
        case eImageFormat::GIF:
        case eImageFormat::HDP:
        case eImageFormat::JXR:
        case eImageFormat::WDP:
        case eImageFormat::ICO:
        case eImageFormat::HEIF:
        case eImageFormat::HEIC:
            hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, m_scratchImg);
            break;

        case eImageFormat::Unknown:
            SetErrorMessage("Unknown image format: '{}'", m_filePath.native());
            return eErrorCode::UnsupportedFile;
    }

    if (FAILED(hr))
    {
        SetErrorMessage("Failed to load image file: '{}'.\n"
                        "message: {}",
                        m_filePath.native(),
                        CreateComErrorMessage(hr));
        return eErrorCode::FileReadFailed;
    }

    if (m_bConvertFormat)
    {
        JAM_ASSERT(m_convertFormat != eFormat::Unknown, "invalid format to convert");

        DXGI_FORMAT format = k_dxgiFormatTable[EnumToInt(m_convertFormat)];

        if (format == DXGI_FORMAT_UNKNOWN)
        {
            SetErrorMessage("Unsupported format to convert: '{}'", EnumToString(m_convertFormat));
            return eErrorCode::RuntimeError;
        }

        hr = DirectX::Convert(m_scratchImg.GetImages(),
                              m_scratchImg.GetImageCount(),
                              m_scratchImg.GetMetadata(),
                              format,
                              DirectX::TEX_FILTER_DEFAULT,
                              m_convertThreshold,
                              m_scratchImg);

        if (FAILED(hr))
        {
            SetErrorMessage("Failed to convert image format to: '{}'", EnumToString(m_convertFormat));
            return eErrorCode::RuntimeError;
        }
    }

    if (m_bForceSRGB)
    {
        DirectX::TexMetadata metadata = m_scratchImg.GetMetadata();
        if (!DirectX::IsSRGB(metadata.format))
        {
            DXGI_FORMAT newFormat = DirectX::MakeSRGB(metadata.format);
            if (newFormat == DXGI_FORMAT_UNKNOWN)
            {
                SetErrorMessage("Failed to convert image format to sRGB: '{}'", EnumToString(m_convertFormat));
                return eErrorCode::RuntimeError;
            }

            hr = DirectX::Convert(m_scratchImg.GetImages(),
                                  m_scratchImg.GetImageCount(),
                                  m_scratchImg.GetMetadata(),
                                  newFormat,
                                  DirectX::TEX_FILTER_DEFAULT,
                                  m_convertThreshold,
                                  m_scratchImg);

            if (FAILED(hr))
            {
                SetErrorMessage("Failed to convert image format to sRGB: '{}'", EnumToString(m_convertFormat));
                return eErrorCode::RuntimeError;
            }
        }
    }

    if (m_bPreMultiplyAlpha)
    {
        hr = DirectX::PremultiplyAlpha(m_scratchImg.GetImages(),
                                       m_scratchImg.GetImageCount(),
                                       m_scratchImg.GetMetadata(),
                                       DirectX::TEX_PMALPHA_DEFAULT,
                                       m_scratchImg);
        if (FAILED(hr))
        {
            SetErrorMessage("Failed to premultiply alpha");
            return eErrorCode::RuntimeError;
        }
    }

    if (m_bGenerateMips)
    {
        bool bIsTex3D = (m_scratchImg.GetMetadata().dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

        if (bIsTex3D)
        {
            hr = DirectX::GenerateMipMaps3D(m_scratchImg.GetImages(),
                                            m_scratchImg.GetImageCount(),
                                            m_scratchImg.GetMetadata(),
                                            DirectX::TEX_FILTER_DEFAULT,
                                            0,
                                            m_scratchImg);
        }
        else
        {
            hr = DirectX::GenerateMipMaps(m_scratchImg.GetImages(),
                                          m_scratchImg.GetImageCount(),
                                          m_scratchImg.GetMetadata(),
                                          DirectX::TEX_FILTER_DEFAULT,
                                          0,
                                          m_scratchImg);
        }

        if (FAILED(hr))
        {
            SetErrorMessage("Failed to generate mipmaps");
            return eErrorCode::RuntimeError;
        }
    }

    if (m_bResize)
    {
        JAM_ASSERT(m_newWidth > 0 && m_newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_scratchImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        hr = DirectX::Resize(m_scratchImg.GetImages(),
                             m_scratchImg.GetImageCount(),
                             m_scratchImg.GetMetadata(),
                             m_newWidth,
                             m_newHeight,
                             DirectX::TEX_FILTER_DEFAULT,
                             m_scratchImg);

        if (FAILED(hr))
        {
            SetErrorMessage("Failed to resize image to {}x{}", m_newWidth, m_newHeight);
            return eErrorCode::RuntimeError;
        }
    }

    if (m_bFlipVertical)
    {
        hr = DirectX::FlipRotate(m_scratchImg.GetImages(),
                                 m_scratchImg.GetImageCount(),
                                 m_scratchImg.GetMetadata(),
                                 DirectX::TEX_FR_FLIP_VERTICAL,
                                 m_scratchImg);

        if (FAILED(hr))
        {
            SetErrorMessage("Failed to flip image vertically");
            return eErrorCode::RuntimeError;
        }
    }

    if (m_bFlipHorizontal)
    {
        hr = DirectX::FlipRotate(m_scratchImg.GetImages(),
                                 m_scratchImg.GetImageCount(),
                                 m_scratchImg.GetMetadata(),
                                 DirectX::TEX_FR_FLIP_HORIZONTAL,
                                 m_scratchImg);

        if (FAILED(hr))
        {
            SetErrorMessage("Failed to flip image horizontally");
            return eErrorCode::RuntimeError;
        }
    }

    return eErrorCode::Ok;
}

void ImageImporter::Reset()
{
    m_filePath.clear();
    m_format = eImageFormat::Unknown;

    m_scratchImg.Release();

    m_bConvertFormat   = false;
    m_bForceSRGB       = false;
    m_convertFormat    = eFormat::Unknown;
    m_convertThreshold = 0.5f;

    m_bGenerateMips = false;

    m_bResize   = false;
    m_newWidth  = 0;
    m_newHeight = 0;

    m_bFlipVertical   = false;
    m_bFlipHorizontal = false;
}

void ImageImporter::SetPath(const fs::path& _filePath)
{
    m_filePath = _filePath;
    m_format   = DetermineFormatFromExtension(_filePath);
    JAM_ASSERT(m_format != eImageFormat::Unknown, "unsupported image format: '{}'", _filePath.native());
}

void ImageImporter::ConvertFormat(const eFormat _format, const float _threshold)
{
    JAM_ASSERT(_format != eFormat::Unknown, "invalid format");

    m_bConvertFormat   = true;
    m_convertFormat    = _format;
    m_convertThreshold = _threshold;
}

void ImageImporter::GenerateMips()
{
    m_bGenerateMips = true;
}

void ImageImporter::Resize(const size_t _newWidth, const size_t _newHeight)
{
    m_bResize   = true;
    m_newWidth  = _newWidth;
    m_newHeight = _newHeight;
}

void ImageImporter::FlipVertical()
{
    m_bFlipVertical = true;
}

void ImageImporter::FlipHorizontal()
{
    m_bFlipHorizontal = true;
}

void ImageImporter::PreMultiplyAlpha()
{
    m_bPreMultiplyAlpha = true;
}

const DirectX::ScratchImage& ImageImporter::GetScratchImage() const
{
    JAM_ASSERT(m_scratchImg.GetImageCount() > 0, "image not loaded");
    return m_scratchImg;
}

}   // namespace jam