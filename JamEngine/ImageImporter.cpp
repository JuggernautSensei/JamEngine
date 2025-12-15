#include "pch.h"

#include "ImageImporter.h"

#include "TextureFormat.h"
#include <DirectXTexEXR.h>

namespace jam
{

bool ImageImporter::Import()
{
    // import
    bool bOk;
    switch (m_importType)
    {
        case eImportType::FromPath:
            bOk = ImportFile_();
            break;

        case eImportType::FromSource:
            bOk = ImportSource_();
            break;

        default:
            ErrorBuffer::Set("invalid import type");
            bOk = false;
            break;
    }

    if (!bOk)
    {
        return false;
    }

    // convert format
    if (m_bConvertFormat)
    {
        JAM_ASSERT(m_convertFormat != eTextureFormat::Unknown, "invalid format to convert");

        const TextureFormatInfo& info = TextureFormatInfoFrom(m_convertFormat);
        if (info.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            ErrorBuffer::Set("invalid format to convert: '{}'", StringFrom(m_convertFormat));
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
            ErrorBuffer::Set("Failed to convert image format: {}", ErrorMessageFrom(hr));
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
                ErrorBuffer::Set("Failed to convert image format to sRGB");
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
                ErrorBuffer::Set("Failed to convert to sRGB: {}", ErrorMessageFrom(hr));
                return false;
            }

            m_scratchImg = std::move(scratchImg);
        }
    }

    // premultiply alpha
    if (m_bPreMultiplyAlpha)
    {
        DirectX::ScratchImage scretchImg;
        const HRESULT         hr = DirectX::PremultiplyAlpha(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            scretchImg);

        if (FAILED(hr))
        {
            ErrorBuffer::Set("Failed to premultiply alpha: {}", ErrorMessageFrom(hr));
            return false;
        }

        m_scratchImg = std::move(scretchImg);
    }

    // resize image
    if (m_bResize)
    {
        JAM_ASSERT(m_newWidth > 0 && m_newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_scratchImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        DirectX::ScratchImage scratchImg;
        const HRESULT         hr = DirectX::Resize(
            m_scratchImg.GetImages(),
            m_scratchImg.GetImageCount(),
            m_scratchImg.GetMetadata(),
            m_newWidth,
            m_newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            scratchImg);

        if (FAILED(hr))
        {
            ErrorBuffer::Set("Failed to resize image to {}x{}: {}", m_newWidth, m_newHeight, ErrorMessageFrom(hr));
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

            DirectX::ScratchImage scratchImg;
            HRESULT               hr;

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
                ErrorBuffer::Set("Failed to generate mipmaps: {}", ErrorMessageFrom(hr));
                return false;
            }
            m_scratchImg = std::move(scratchImg);
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
            ErrorBuffer::Set("Failed to flip image vertically: {}", ErrorMessageFrom(hr));
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
            ErrorBuffer::Set("Failed to flip image horizontally: {}", ErrorMessageFrom(hr));
            return false;
        }

        m_scratchImg = std::move(img);
    }

    return true;
}

void ImageImporter::SetPath(const fs::path& _path)
{
    m_path       = _path;
    m_importType = eImportType::FromPath;
}

void ImageImporter::SetSource(const ConstMemView _source, const eImageFormat _imageFormat)
{
    m_source       = _source;
    m_sourceFormat = _imageFormat;
    m_importType   = eImportType::FromSource;
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

bool ImageImporter::ImportFile_()
{
    JAM_ASSERT(!m_path.empty(), "file path is empty");

    const std::wstring wPath  = m_path.wstring();
    const eImageFormat format = ImageFormatFrom(m_path);
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
            ErrorBuffer::Set("Unknown image format: {}", ErrorMessageFrom(format));
            return false;
    }

    if (FAILED(hr))
    {
        ErrorBuffer::Set("Failed to load image file '{}': {}", m_path.native(), ErrorMessageFrom(hr));
        return false;
    }

    m_scratchImg = std::move(scratchImg);
    return true;
}

bool ImageImporter::ImportSource_()
{
    JAM_ASSERT(m_source.IsValid(), "memory is empty");

    DirectX::ScratchImage scratchImg;
    HRESULT               hr;

    const Byte*  pData = m_source.GetPointer();
    const size_t size  = m_source.GetByteWidth();
    switch (m_sourceFormat)
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
            ErrorBuffer::Set("Unsupported image format for memory import: '{}'", ErrorMessageFrom(m_sourceFormat));
            return false;
    }

    if (FAILED(hr))
    {
        ErrorBuffer::Set("Failed to load image from memory: {}", ErrorMessageFrom(hr));
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