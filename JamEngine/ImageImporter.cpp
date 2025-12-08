#include "pch.h"

#include "ImageImporter.h"

#include "TextureFormat.h"
#include <DirectXTexEXR.h>

namespace jam
{

bool ImageImporter::Import()
{
    // import
    bool bSucceeded;
    switch (m_importType)
    {
        case eImportType::FromPath:
            bSucceeded = ImportFromPath_();
            break;

        case eImportType::FromMemory:
            bSucceeded = ImportFromMemory_();
            break;

        default:
            SetErrorString("invalid import type");
            bSucceeded = false;
    }

    if (!bSucceeded)
    {
        return false;
    }

    // convert format 
    if (m_bConvertFormat)
    {
        JAM_ASSERT(m_convertFormat != eTextureFormat::Unknown, "invalid format to convert");

        const TextureFormatInfo& info = GetTextureFormatInfo(m_convertFormat);
        if (info.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            SetErrorString("invalid format to convert: '{}'", EnumToString(m_convertFormat));
            return false;
        }

        DirectX::ScratchImage scratchImg;
        const HRESULT         hr = DirectX::Convert(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            info.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            m_convertThreshold,
            scratchImg);

        if (FAILED(hr))
        {
            SetErrorString("Failed to convert image format: {}", CreateComErrorString(hr));
            return false;
        }

        m_scratchImg = std::move(scratchImg);
    }

    // force sRGB 
    if (m_bForceSRGB)
    {
        const DirectX::TexMetadata& metadata = m_scratchImg.GetMetadata();
        if (!DirectX::IsSRGB(metadata.format))
        {
            const DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);
            if (srgbFormat == DXGI_FORMAT_UNKNOWN)
            {
                SetErrorString("Failed to convert image format to sRGB");
                return false;
            }

            DirectX::ScratchImage scratchImg;
            const HRESULT         hr = DirectX::Convert(
                m_scratchImg.GetImages(),
                m_scratchImg.GetImageCount(),
                m_scratchImg.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                m_convertThreshold,
                scratchImg);

            if (FAILED(hr))
            {
                SetErrorString("Failed to convert to sRGB: {}", CreateComErrorString(hr));
                return false;
            }

            m_scratchImg = std::move(scratchImg);
        }
    }

    // premultiply alpha 
    if (m_bPreMultiplyAlpha)
    {
        DirectX::ScratchImage img;
        const HRESULT         hr = DirectX::PremultiplyAlpha(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            img);

        if (FAILED(hr))
        {
            SetErrorString("Failed to premultiply alpha: {}", CreateComErrorString(hr));
            return false;
        }

        m_scratchImg = std::move(img);
    }

    // resize image 
    if (m_bResize)
    {
        JAM_ASSERT(m_newWidth > 0 && m_newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_scratchImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        DirectX::ScratchImage img;
        const HRESULT         hr = DirectX::Resize(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            m_newWidth,
            m_newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            img);

        if (FAILED(hr))
        {
            SetErrorString("Failed to resize image to {}x{}: {}", m_newWidth, m_newHeight, CreateComErrorString(hr));
            return false;
        }

        m_scratchImg = std::move(img);
    }

    // drop or generate mips 
    if (m_bDropMips)
    {
        const DirectX::TexMetadata& metadata = m_scratchImg.GetMetadata();

        if (metadata.mipLevels > 1)
        {
            DirectX::TexMetadata newMetadata = metadata;
            newMetadata.mipLevels            = 1;

            DirectX::ScratchImage scratchImg;
            const HRESULT         hr = scratchImg.Initialize(newMetadata);
            if (FAILED(hr))
            {
                SetErrorString("Failed to initialize scratch image for dropping mips: {}", CreateComErrorString(hr));
                return false;
            }

            for (size_t item = 0; item < metadata.arraySize; ++item)
            {
                const DirectX::Image* srcImage = m_scratchImg.GetImage(0, item, 0);
                const DirectX::Image* dstImage = scratchImg.GetImage(0, item, 0);

                if (srcImage && dstImage)
                {
                    memcpy(dstImage->pixels, srcImage->pixels, srcImage->slicePitch);
                }
            }

            m_scratchImg = std::move(scratchImg);
        }
    }
    else
    {
        const DirectX::TexMetadata& metadata = m_scratchImg.GetMetadata();

        // Generate mipmaps if there is only one level
        if (metadata.mipLevels == 1)
        {
            const bool bIs3d = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

            DirectX::ScratchImage img;
            HRESULT               hr;

            if (bIs3d)
            {
                hr = DirectX::GenerateMipMaps3D(
                    m_scratchImg.GetImages(),
                    m_scratchImg.GetImageCount(),
                    m_scratchImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    img);
            }
            else
            {
                hr = DirectX::GenerateMipMaps(
                    m_scratchImg.GetImages(),
                    m_scratchImg.GetImageCount(),
                    m_scratchImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    img);
            }

            if (FAILED(hr))
            {
                SetErrorString("Failed to generate mipmaps: {}", CreateComErrorString(hr));
                return false;
            }
            m_scratchImg = std::move(img);
        }
    }

    // flip vertical
    if (m_bFlipVertical)
    {
        DirectX::ScratchImage img;
        const HRESULT         hr = DirectX::FlipRotate(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_VERTICAL,
            img);

        if (FAILED(hr))
        {
            SetErrorString("Failed to flip image vertically: {}", CreateComErrorString(hr));
            return false;
        }

        m_scratchImg = std::move(img);
    }

    // flip horizontal
    if (m_bFlipHorizontal)
    {
        DirectX::ScratchImage img;
        const HRESULT         hr = DirectX::FlipRotate(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_HORIZONTAL,
            img);

        if (FAILED(hr))
        {
            SetErrorString("Failed to flip image horizontally: {}", CreateComErrorString(hr));
            return false;
        }

        m_scratchImg = std::move(img);
    }

    return true;
}

void ImageImporter::SetPath(const fs::path& _filePath)
{
    m_path       = _filePath;
    m_importType = eImportType::FromPath;
}

void ImageImporter::SetMemory(const ConstMemView _mem, const eImageFormat _imageFormat)
{
    m_imageSource       = _mem;
    m_imageSourceFormat = _imageFormat;
    m_importType        = eImportType::FromMemory;
}

void ImageImporter::ConvertFormat(const bool _bEnable, const eTextureFormat _format, const float _threshold)
{
    JAM_ASSERT(_format != eTextureFormat::Unknown, "invalid format");
    m_bConvertFormat   = _bEnable;
    m_convertFormat    = _format;
    m_convertThreshold = _threshold;
}

void ImageImporter::ForceSRGB(const bool _bEnable)
{
    m_bForceSRGB = _bEnable;
}

void ImageImporter::DropMips(const bool _bEnable)
{
    m_bDropMips = _bEnable;
}

void ImageImporter::Resize(const bool _bEnable, const size_t _newWidth, const size_t _newHeight)
{
    m_bResize   = _bEnable;
    m_newWidth  = _newWidth;
    m_newHeight = _newHeight;
}

void ImageImporter::FlipVertical(const bool _bEnable)
{
    m_bFlipVertical = _bEnable;
}

void ImageImporter::FlipHorizontal(const bool _bEnable)
{
    m_bFlipHorizontal = _bEnable;
}

void ImageImporter::PreMultiplyAlpha(const bool _bEnable)
{
    m_bPreMultiplyAlpha = _bEnable;
}

bool ImageImporter::ImportFromPath_()
{
    JAM_ASSERT(!m_path.empty(), "file path is empty");

    const std::wstring wPath  = m_path.wstring();
    const eImageFormat format = ImageFormatFromPath(m_path);
    HRESULT            hr;

    DirectX::ScratchImage scratchImg;
    switch (format)
    {
        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(wPath.c_str(), nullptr, scratchImg);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, scratchImg);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(wPath.c_str(), nullptr, scratchImg);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(wPath.c_str(), nullptr, scratchImg);
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
            hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, scratchImg);
            break;

        default:
            SetErrorString("Unknown image format: {}", EnumToString(format));
            return false;
    }

    if (FAILED(hr))
    {
        SetErrorString("Failed to load image file '{}': {}", m_path.native(), CreateComErrorString(hr));
        return false;
    }

    m_scratchImg = std::move(scratchImg);
    return true;
}

bool ImageImporter::ImportFromMemory_()
{
    JAM_ASSERT(m_imageSource.IsValid(), "memory is empty");
    JAM_ASSERT(m_imageSourceFormat != eImageFormat::Unknown, "image format is unknown");

    DirectX::ScratchImage scratchImg;
    HRESULT               hr;

    const Byte*  pData = m_imageSource.GetDataPtr();
    const size_t size  = m_imageSource.GetByteWidth();
    switch (m_imageSourceFormat)
    {
        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSMemory(pData, size, DirectX::DDS_FLAGS_NONE, nullptr, scratchImg);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRMemory(pData, size, nullptr, scratchImg);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAMemory(pData, size, nullptr, scratchImg);
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
            hr = DirectX::LoadFromWICMemory(pData, size, DirectX::WIC_FLAGS_NONE, nullptr, scratchImg);
            break;

        default:
            SetErrorString("Unsupported image format for memory import: '{}'", EnumToString(m_imageSourceFormat));
            return false;
    }

    if (FAILED(hr))
    {
        SetErrorString("Failed to load image from memory: {}", CreateComErrorString(hr));
        return false;
    }

    m_scratchImg = std::move(scratchImg);
    return true;
}

const DirectX::ScratchImage& ImageImporter::GetScratchImage() const
{
    JAM_ASSERT(m_scratchImg.GetImageCount() > 0, "image not loaded");
    return m_scratchImg;
}

}   // namespace jam