#include "pch.h"

#include "ImageImporter.h"

#include "TextureFormat.h"
#include <DirectXTexEXR.h>

namespace jam
{

bool ImageImporter::Import()
{
    // import
    bool bSucceeded = false;
    switch (m_importType)
    {
        case eImportType::FromPath:
            bSucceeded = ImportFile_();
            break;

        case eImportType::FromSource:
            bSucceeded = ImportSource_();
            break;

        default:
            GlobalBuffer::SetLastError("invalid import type");
            bSucceeded = false;
            break;
    }

    if (!bSucceeded)
    {
        return false;
    }

    // post process

    // convert format
    if (m_bConvertFormat)
    {
        JAM_ASSERT(m_convertFormat != eTextureFormat::Unknown, "invalid format to convert");

        const TextureFormatInfo& info = TextureFormatInfoFrom(m_convertFormat);
        if (info.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            GlobalBuffer::SetLastError("invalid format to convert: '{}'", StringFrom(m_convertFormat));
            return false;
        }

        DirectX::ScratchImage scratchImg = {};
        const HRESULT         hr         = DirectX::Convert(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            info.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            m_convertThreshold,
            scratchImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to convert image format: {}", StringFrom(hr));
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
                GlobalBuffer::SetLastError("Failed to convert image format to sRGB");
                return false;
            }

            DirectX::ScratchImage scratchImg = {};
            const HRESULT         hr         = DirectX::Convert(
                m_scratchImg.GetImages(),
                m_scratchImg.GetImageCount(),
                m_scratchImg.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                m_convertThreshold,
                scratchImg);

            if (FAILED(hr))
            {
                GlobalBuffer::SetLastError("Failed to convert to sRGB: {}", StringFrom(hr));
                return false;
            }

            m_scratchImg = std::move(scratchImg);
        }
    }

    // premultiply alpha
    if (m_bPreMultiplyAlpha)
    {
        DirectX::ScratchImage scretchImg = {};
        const HRESULT         hr         = DirectX::PremultiplyAlpha(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            scretchImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to premultiply alpha: {}", StringFrom(hr));
            return false;
        }

        m_scratchImg = std::move(scretchImg);
    }

    // resize image
    if (m_bResize)
    {
        JAM_ASSERT(m_newWidth > 0 && m_newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_scratchImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        DirectX::ScratchImage scratchImg = {};
        const HRESULT         hr         = DirectX::Resize(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            m_newWidth,
            m_newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            scratchImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to resize image to {}x{}: {}", m_newWidth, m_newHeight, StringFrom(hr));
            return false;
        }

        m_scratchImg = std::move(scratchImg);
    }

    // drop or generate mips
    if (m_bGenerateMips)
    {
        const DirectX::TexMetadata& metadata = m_scratchImg.GetMetadata();

        // Generate mipmaps if there is only one level
        if (metadata.mipLevels == 1)
        {
            const bool bIs3d = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

            DirectX::ScratchImage scratchImg = {};
            HRESULT               hr         = E_FAIL;

            if (bIs3d)
            {
                hr = DirectX::GenerateMipMaps3D(
                    m_scratchImg.GetImages(),
                    m_scratchImg.GetImageCount(),
                    m_scratchImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    scratchImg);
            }
            else
            {
                hr = DirectX::GenerateMipMaps(
                    m_scratchImg.GetImages(),
                    m_scratchImg.GetImageCount(),
                    m_scratchImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    scratchImg);
            }

            if (FAILED(hr))
            {
                GlobalBuffer::SetLastError("Failed to generate mipmaps: {}", StringFrom(hr));
                return false;
            }

            m_scratchImg = std::move(scratchImg);
        }
    }

    // flip vertical
    if (m_bFlipVertical)
    {
        DirectX::ScratchImage scratchImg = {};
        const HRESULT         hr         = DirectX::FlipRotate(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_VERTICAL,
            scratchImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image vertically: {}", StringFrom(hr));
            return false;
        }

        m_scratchImg = std::move(scratchImg);
    }

    // flip horizontal
    if (m_bFlipHorizontal)
    {
        DirectX::ScratchImage scratchImg = {};
        const HRESULT         hr         = DirectX::FlipRotate(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_HORIZONTAL,
            scratchImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image horizontally: {}", StringFrom(hr));
            return false;
        }

        m_scratchImg = std::move(scratchImg);
    }

    return true;
}

void ImageImporter::SetPath(const fs::path& _path)
{
    m_path       = _path;
    m_importType = eImportType::FromPath;
}

void ImageImporter::SetMemory(const CMemView _memory, const eImageFormat _imageFormat)
{
    m_mem        = _memory;
    m_memFormat  = _imageFormat;
    m_importType = eImportType::FromSource;
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

void ImageImporter::GenerateMips(const bool _bEnable)
{
    m_bGenerateMips = _bEnable;
}

void ImageImporter::Resize(
    const bool  _bEnable,
    const Int32 _newWidth,
    const Int32 _newHeight)
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

bool ImageImporter::ImportFile_()
{
    JAM_ASSERT(!m_path.empty(), "file path is empty");

    const Optional<eImageFormat> optFormat = ImageFormatFrom(m_path);
    if (!optFormat)
    {
        GlobalBuffer::SetLastError("Unknown image format for file: '{}'", m_path.native());
        return false;
    }

    DirectX::ScratchImage scratchImg = {};
    const eImageFormat    format     = optFormat.value();
    const std::wstring    path       = m_path.wstring();
    HRESULT               hr         = E_FAIL;

    switch (format)
    {
        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(path.c_str(), nullptr, scratchImg);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, scratchImg);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(path.c_str(), nullptr, scratchImg);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(path.c_str(), nullptr, scratchImg);
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
            hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, scratchImg);
            break;

        default:
            GlobalBuffer::SetLastError("Unknown image format: {}", StringFrom(format));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image file '{}': {}", m_path.native(), StringFrom(hr));
        return false;
    }

    m_scratchImg = std::move(scratchImg);
    return true;
}

bool ImageImporter::ImportSource_()
{
    JAM_ASSERT(m_mem.IsEmpty(), "memory is empty");

    DirectX::ScratchImage scratchImg = {};
    const Byte*           pMem       = m_mem.GetPtr();
    const Int32           byteWidth  = m_mem.GetByteWidth();
    HRESULT               hr         = E_FAIL;

    switch (m_memFormat)
    {
        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSMemory(pMem, byteWidth, DirectX::DDS_FLAGS_NONE, nullptr, scratchImg);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRMemory(pMem, byteWidth, nullptr, scratchImg);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAMemory(pMem, byteWidth, DirectX::TGA_FLAGS_NONE, nullptr, scratchImg);
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
            hr = DirectX::LoadFromWICMemory(pMem, byteWidth, DirectX::WIC_FLAGS_NONE, nullptr, scratchImg);
            break;

        default:
            GlobalBuffer::SetLastError("Unsupported image format for memory import: '{}'", StringFrom(m_memFormat));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image from memory: {}", StringFrom(hr));
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