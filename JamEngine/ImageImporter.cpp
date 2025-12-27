#include "pch.h"

#include "ImageImporter.h"

#include "TextureFormat.h"
#include <DirectXTexEXR.h>

#include "Graphics.h"

namespace jam
{

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

Optional<DirectX::ScratchImage> ImageImporter::PostProcess_(
    DirectX::ScratchImage&& _srcScratchImg)
{
    DirectX::ScratchImage outputImg = std::move(_srcScratchImg);

    // convert format
    if (m_bConvertFormat)
    {
        JAM_ASSERT(m_convertFormat != eTextureFormat::Unknown, "invalid format to convert");

        const TextureFormatInfo& texInfo = TextureFormatInfoFrom(m_convertFormat);
        if (texInfo.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            GlobalBuffer::SetLastError("invalid format to convert: '{}'", StringFrom(m_convertFormat));
            return k_nullopt;
        }

        DirectX::ScratchImage resultImg;
        const HRESULT         hr = DirectX::Convert(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            texInfo.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            m_convertThreshold,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to convert image format: {}", StringFrom(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // force sRGB
    if (m_bForceSRGB)
    {
        const DirectX::TexMetadata& metadata = outputImg.GetMetadata();
        if (!DirectX::IsSRGB(metadata.format))
        {
            const DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);
            if (srgbFormat == DXGI_FORMAT_UNKNOWN)
            {
                GlobalBuffer::SetLastError("Failed to convert image format to sRGB");
                return k_nullopt;
            }

            DirectX::ScratchImage resultImg;
            const HRESULT         hr = DirectX::Convert(
                outputImg.GetImages(),
                outputImg.GetImageCount(),
                outputImg.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                m_convertThreshold,
                resultImg);

            if (FAILED(hr))
            {
                GlobalBuffer::SetLastError("Failed to convert to sRGB: {}", StringFrom(hr));
                return k_nullopt;
            }

            outputImg = std::move(resultImg);
        }
    }

    // premultiply alpha
    if (m_bPreMultiplyAlpha)
    {
        DirectX::ScratchImage resultImg;
        const HRESULT         hr = DirectX::PremultiplyAlpha(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to premultiply alpha: {}", StringFrom(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // resize image
    if (m_bResize)
    {
        JAM_ASSERT(m_newWidth > 0 && m_newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(outputImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        DirectX::ScratchImage resultImg;
        const HRESULT         hr = DirectX::Resize(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            m_newWidth,
            m_newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to resize image to {}x{}: {}", m_newWidth, m_newHeight, StringFrom(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // drop or generate mips
    if (m_bGenerateMips)
    {
        const DirectX::TexMetadata& metadata = outputImg.GetMetadata();

        // Generate mipmaps if there is only one level
        if (metadata.mipLevels == 1)
        {
            const bool bIs3d = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

            DirectX::ScratchImage resultImg;
            HRESULT               hr;

            if (bIs3d)
            {
                hr = DirectX::GenerateMipMaps3D(
                    outputImg.GetImages(),
                    outputImg.GetImageCount(),
                    outputImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    resultImg);
            }
            else
            {
                hr = DirectX::GenerateMipMaps(
                    outputImg.GetImages(),
                    outputImg.GetImageCount(),
                    outputImg.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    resultImg);
            }

            if (FAILED(hr))
            {
                GlobalBuffer::SetLastError("Failed to generate mipmaps: {}", StringFrom(hr));
                return k_nullopt;
            }

            outputImg = std::move(resultImg);
        }
    }

    // flip vertical
    if (m_bFlipVertical)
    {
        DirectX::ScratchImage resultImg;
        const HRESULT         hr = DirectX::FlipRotate(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_VERTICAL,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image vertically: {}", StringFrom(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // flip horizontal
    if (m_bFlipHorizontal)
    {
        DirectX::ScratchImage resultImg;
        const HRESULT         hr = DirectX::FlipRotate(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_HORIZONTAL,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image horizontally: {}", StringFrom(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    return outputImg;
}

bool ImageImporter::Import(
    const fs::path& _path)
{
    m_texh = k_invalidHandle;   // clean up

    // determine image format
    const Optional<eImageFormat> optFormat = ImageFormatFrom(_path);
    if (!optFormat)
    {
        GlobalBuffer::SetLastError("Unknown image format for file: '{}'", _path.native());
        return false;
    }

    // load from file
    DirectX::ScratchImage scratchImg;
    const eImageFormat    format = optFormat.value();
    const std::wstring    wPath  = _path.wstring();
    HRESULT               hr;

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
            GlobalBuffer::SetLastError("Unknown image format: {}", StringFrom(format));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image file '{}': {}", _path.native(), StringFrom(hr));
        return false;
    }

    // post process
    Optional<DirectX::ScratchImage> optProcessedImg = PostProcess_(std::move(scratchImg));
    if (!optProcessedImg)
    {
        return false;
    }

    m_texh = Graphics::LoadTexture(optProcessedImg.value());
    return true;
}

bool ImageImporter::ImportMemory(
    const CMemView     _mem,
    const eImageFormat _imageFormat)
{
    JAM_ASSERT(_mem.IsEmpty(), "memory is empty");

    m_texh = k_invalidHandle;   // clean up

    // load from memory
    DirectX::ScratchImage scratchImg;
    const Byte*           pMem      = _mem.GetPtr();
    const Int32           byteWidth = _mem.GetByteWidth();
    HRESULT               hr;

    switch (_imageFormat)
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
            GlobalBuffer::SetLastError("Unsupported image format for memory import: '{}'", StringFrom(_imageFormat));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image from memory: {}", StringFrom(hr));
        return false;
    }

    // post process
    Optional<DirectX::ScratchImage> optProcessedImg = PostProcess_(std::move(scratchImg));
    if (!optProcessedImg)
    {
        return false;
    }

    m_texh = Graphics::LoadTexture(optProcessedImg.value());
    return true;
}

TextureHandle ImageImporter::GetTexture() const
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(m_texh), "texture is not initialized");
    return m_texh;
}

}   // namespace jam