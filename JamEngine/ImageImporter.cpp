#include "pch.h"

#include "ImageImporter.h"

#include "TextureFormat.h"
#include <DirectXTexEXR.h>

#include "Graphics.h"
#include "WindowsUtilities.h"

namespace jam
{

void ImageImporter::ConvertFormat(
    const bool           _bEnable,
    const eTextureFormat _format,
    const float          _threshold)
{
    JAM_ASSERT(_format != eTextureFormat::Unknown, "invalid format");
    m_options.bConvertFormat   = _bEnable;
    m_options.convertFormat    = _format;
    m_options.convertThreshold = _threshold;
}

void ImageImporter::ForceSRGB(
    const bool _bEnable)
{
    m_options.bForceSRGB = _bEnable;
}

void ImageImporter::GenerateMips(
    const bool _bEnable)
{
    m_options.bGenerateMips = _bEnable;
}

void ImageImporter::Resize(
    const bool  _bEnable,
    const Int32 _newWidth,
    const Int32 _newHeight)
{
    m_options.bResize   = _bEnable;
    m_options.newWidth  = _newWidth;
    m_options.newHeight = _newHeight;
}

void ImageImporter::FlipVertical(const bool _bEnable)
{
    m_options.bFlipVertical = _bEnable;
}

void ImageImporter::FlipHorizontal(const bool _bEnable)
{
    m_options.bFlipHorizontal = _bEnable;
}

void ImageImporter::PreMultiplyAlpha(const bool _bEnable)
{
    m_options.bPreMultiplyAlpha = _bEnable;
}

void ImageImporter::SetOptions(
    const ImageImporterOptions& _options)
{
    m_options = _options;
}

Optional<DirectX::ScratchImage> ImageImporter::PostProcess_(
    DirectX::ScratchImage&& _srcScratchImg)
{
    DirectX::ScratchImage outputImg = std::move(_srcScratchImg);

    // convert format
    if (m_options.bConvertFormat)
    {
        JAM_ASSERT(m_options.convertFormat != eTextureFormat::Unknown, "invalid format to convert");

        const TextureFormatInfo& texInfo = TextureFormatInfoFrom(m_options.convertFormat);
        if (texInfo.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            GlobalBuffer::SetLastError("invalid format to convert: '{}'", texInfo.name);
            return k_nullopt;
        }

        DirectX::ScratchImage resultImg = {};
        const HRESULT         hr        = DirectX::Convert(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            texInfo.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            m_options.convertThreshold,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to convert image format: {}", CreateErrorMessageFromHRESULT(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // force sRGB
    if (m_options.bForceSRGB)
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

            DirectX::ScratchImage resultImg = {};
            const HRESULT         hr        = DirectX::Convert(
                outputImg.GetImages(),
                outputImg.GetImageCount(),
                outputImg.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                m_options.convertThreshold,
                resultImg);

            if (FAILED(hr))
            {
                GlobalBuffer::SetLastError("Failed to convert to sRGB: {}", CreateErrorMessageFromHRESULT(hr));
                return k_nullopt;
            }

            outputImg = std::move(resultImg);
        }
    }

    // premultiply alpha
    if (m_options.bPreMultiplyAlpha)
    {
        DirectX::ScratchImage resultImg = {};
        const HRESULT         hr        = DirectX::PremultiplyAlpha(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to premultiply alpha: {}", CreateErrorMessageFromHRESULT(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // resize image
    if (m_options.bResize)
    {
        JAM_ASSERT(m_options.newWidth > 0 && m_options.newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(outputImg.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        DirectX::ScratchImage resultImg = {};
        const HRESULT         hr        = DirectX::Resize(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            m_options.newWidth,
            m_options.newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to resize image to {}x{}: {}", m_options.newWidth, m_options.newHeight, CreateErrorMessageFromHRESULT(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // drop or generate mips
    if (m_options.bGenerateMips)
    {
        const DirectX::TexMetadata& metadata = outputImg.GetMetadata();

        // Generate mipmaps if there is only one level
        if (metadata.mipLevels == 1)
        {
            const bool bIs3d = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

            DirectX::ScratchImage resultImg = {};
            HRESULT               hr        = S_OK;

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
                GlobalBuffer::SetLastError("Failed to generate mipmaps: {}", CreateErrorMessageFromHRESULT(hr));
                return k_nullopt;
            }

            outputImg = std::move(resultImg);
        }
    }

    // flip vertical
    if (m_options.bFlipVertical)
    {
        DirectX::ScratchImage resultImg = {};
        const HRESULT         hr        = DirectX::FlipRotate(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_VERTICAL,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image vertically: {}", CreateErrorMessageFromHRESULT(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    // flip horizontal
    if (m_options.bFlipHorizontal)
    {
        DirectX::ScratchImage resultImg = {};
        const HRESULT         hr        = DirectX::FlipRotate(
            outputImg.GetImages(),
            outputImg.GetImageCount(),
            outputImg.GetMetadata(),
            DirectX::TEX_FR_FLIP_HORIZONTAL,
            resultImg);

        if (FAILED(hr))
        {
            GlobalBuffer::SetLastError("Failed to flip image horizontally: {}", CreateErrorMessageFromHRESULT(hr));
            return k_nullopt;
        }

        outputImg = std::move(resultImg);
    }

    return outputImg;
}

bool ImageImporter::Import(
    const fs::path& _path)
{
    eImageFormat format = {};
    if (!MoveIfHasValue(ImageFormatFrom(_path), format))
    {
        GlobalBuffer::SetLastError("Unknown image format for file: '{}'", _path.native());
        return false;
    }

    return Import(_path, format);
}

bool ImageImporter::Import(
    const fs::path&    _path,
    const eImageFormat _format)
{
    // load from file
    DirectX::ScratchImage scratchImg = {};
    const std::wstring    wPath      = _path.wstring();
    HRESULT               hr         = S_OK;

    switch (_format)
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
            GlobalBuffer::SetLastError("Unknown image format: {}", ExtensionOf(_format));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image file '{}': {}", _path.native(), CreateErrorMessageFromHRESULT(hr));
        return false;
    }

    // post process
    if (!MoveIfHasValue(PostProcess_(std::move(scratchImg)), m_scratchImg))
    {
        return false;
    }

    return true;
}

bool ImageImporter::ImportMemory(
    const CMemView     _mem,
    const eImageFormat _format)
{
    JAM_ASSERT(_mem.IsEmpty(), "memory is empty");

    // load from memory
    DirectX::ScratchImage scratchImg = {};
    const Byte*           pMem       = _mem.GetPtr();
    const Int32           byteWidth  = _mem.GetByteWidth();
    HRESULT               hr         = S_OK;

    switch (_format)
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
            GlobalBuffer::SetLastError("Unsupported image format for memory import: '{}'", ExtensionOf(_format));
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to load image from memory: {}", CreateErrorMessageFromHRESULT(hr));
        return false;
    }

    // post process
    if (!MoveIfHasValue(PostProcess_(std::move(scratchImg)), m_scratchImg))
    {
        return false;
    }

    return true;
}

const DirectX::ScratchImage& ImageImporter::GetScratchImage() const
{
    JAM_ASSERT(m_scratchImg.GetImageCount() > 0, "scratch image is not initialized");
    return m_scratchImg;
}

}   // namespace jam