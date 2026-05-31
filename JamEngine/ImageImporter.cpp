#include "pch.h"
#include "ImageImporter.h"

#include <DirectXTexEXR.h>

#include "WindowsUtilities.h"

namespace Jam
{

Expect<ImageImporter> ImageImporter::ImportFromFile(
    const StringView         _path,
    const eImageFormat       _format,
    const ImageImporterInfo& _info)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _format, _info))
    {
        return err;
    }

    return importer;
}

Expect<ImageImporter> ImageImporter::ImportFromFile(
    const StringView         _path,
    const ImageImporterInfo& _info)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _info))
    {
        return err;
    }

    return importer;
}

Expect<ImageImporter> ImageImporter::ImportFromMemory(
    const MemoryView         _mem,
    const eImageFormat       _format,
    const ImageImporterInfo& _info)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromMemory_(_mem, _format, _info))
    {
        return err;
    }

    return importer;
}

const ImageContainer& ImageImporter::GetImageContainer()
{
    return m_container;
}

Error ImageImporter::ImportFromFile_(
    StringView               _path,
    const eImageFormat       _format,
    const ImageImporterInfo& _info)
{
    // load from file
    const WString path = WString::Encoded(_path);

    HRESULT hr = S_OK;
    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(path.GetPtr(), nullptr, m_container);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(path.GetPtr(), DirectX::DDS_FLAGS_NONE, nullptr, m_container);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(path.GetPtr(), nullptr, m_container);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(path.GetPtr(), nullptr, m_container);
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
            hr = DirectX::LoadFromWICFile(path.GetPtr(), DirectX::WIC_FLAGS_NONE, nullptr, m_container);
            break;

        default:
            JAM_UNREACHABLE;
    }

    if (FAILED(hr))
    {
        JAM_LOG_ERROR("Failed to load image file '{}': {}", _path, MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        return Error::ImportFailed;
    }

    // post process
    return PostProcess_(_info);
}

Error ImageImporter::ImportFromFile_(
    const StringView         _path,
    const ImageImporterInfo& _info)
{
    Optional<eImageFormat> format = ParseAsImageFormat(_path.GetExtension());
    if (!format)
    {
        JAM_LOG_ERROR("Unsupported image format for file import: '{}'", _path.GetExtension());
        return Error::UnsupportedFormat;
    }

    return ImportFromFile_(_path, format.GetValue(), _info);
}

Error ImageImporter::ImportFromMemory_(
    const MemoryView         _mem,
    const eImageFormat       _format,
    const ImageImporterInfo& _info)
{
    // load from memory
    ImageContainer container = {};
    const Byte*    pMem      = _mem.GetPtr();
    const SizeT    byteWidth = _mem.GetByteWidth();
    HRESULT        hr        = S_OK;

    switch (_format)
    {
        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSMemory(pMem, byteWidth, DirectX::DDS_FLAGS_NONE, nullptr, container);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRMemory(pMem, byteWidth, nullptr, container);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAMemory(pMem, byteWidth, DirectX::TGA_FLAGS_NONE, nullptr, container);
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
            hr = DirectX::LoadFromWICMemory(pMem, byteWidth, DirectX::WIC_FLAGS_NONE, nullptr, container);
            break;

        default:
            JAM_LOG_ERROR("Unsupported image format for memory import: '{}'", ExtensionOf(_format));
            return Error::UnsupportedFormat;
    }

    if (FAILED(hr))
    {
        JAM_LOG_ERROR("Failed to load image from memory: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        return Error::ImportFailed;
    }

    // post process
    return PostProcess_(_info);
}

Error ImageImporter::PostProcess_(
    const ImageImporterInfo& _info)
{
    // convert format
    if (_info.bConvertFormat)
    {
        const TextureFormatInfo& texInfo = TextureFormatInfoFrom(_info.convertFormat);
        if (texInfo.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            JAM_LOG_ERROR("invalid format to convert: '{}'", NameOf(_info.convertFormat));
            return Error::InvalidArgument;
        }

        ImageContainer result = {};
        const HRESULT  hr     = DirectX::Convert(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            texInfo.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            _info.convertThreshold,
            result);

        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to convert image format: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // force sRGB
    if (_info.bForceSRGB)
    {
        const DirectX::TexMetadata& metadata = m_container.GetMetadata();
        if (!DirectX::IsSRGB(metadata.format))
        {
            const DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);
            if (srgbFormat == DXGI_FORMAT_UNKNOWN)
            {
                JAM_LOG_ERROR("Failed to convert image format to sRGB");
                return Error::InvalidArgument;
            }

            ImageContainer result = {};
            const HRESULT  hr     = DirectX::Convert(
                m_container.GetImages(),
                m_container.GetImageCount(),
                m_container.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                _info.convertThreshold,
                result);

            if (FAILED(hr))
            {
                JAM_LOG_ERROR("Failed to convert to sRGB: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
                return Error::RuntimeError;
            }

            m_container = std::move(result);
        }
    }

    // premultiply alpha
    if (_info.bPreMultiplyAlpha)
    {
        ImageContainer result = {};
        const HRESULT  hr     = DirectX::PremultiplyAlpha(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            DirectX::TEX_PMALPHA_DEFAULT,
            result);

        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to premultiply alpha: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // resize image
    if (_info.bResize)
    {
        JAM_ASSERT(_info.newWidth > 0 && _info.newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_container.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        ImageContainer result = {};
        const HRESULT  hr     = DirectX::Resize(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            _info.newWidth,
            _info.newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            result);

        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to resize image to {}x{}: {}", _info.newWidth, _info.newHeight, MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // drop or generate mips
    if (_info.bGenerateMips)
    {
        const DirectX::TexMetadata& metadata = m_container.GetMetadata();

        // Generate mipmaps if there is only one level
        if (metadata.mipLevels == 1)
        {
            const bool bIs3d = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D);

            ImageContainer result = {};
            HRESULT        hr     = S_OK;

            if (bIs3d)
            {
                hr = DirectX::GenerateMipMaps3D(
                    m_container.GetImages(),
                    m_container.GetImageCount(),
                    m_container.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    result);
            }
            else
            {
                hr = DirectX::GenerateMipMaps(
                    m_container.GetImages(),
                    m_container.GetImageCount(),
                    m_container.GetMetadata(),
                    DirectX::TEX_FILTER_DEFAULT,
                    0,
                    result);
            }

            if (FAILED(hr))
            {
                JAM_LOG_ERROR("Failed to generate mipmaps: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
                return Error::RuntimeError;
            }

            m_container = std::move(result);
        }
    }

    // flip vertical
    if (_info.bFlipVertical)
    {
        ImageContainer result = {};
        const HRESULT  hr     = DirectX::FlipRotate(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            DirectX::TEX_FR_FLIP_VERTICAL,
            result);

        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to flip image vertically: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // flip horizontal
    if (_info.bFlipHorizontal)
    {
        ImageContainer result = {};
        const HRESULT  hr     = DirectX::FlipRotate(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            DirectX::TEX_FR_FLIP_HORIZONTAL,
            result);

        if (FAILED(hr))
        {
            JAM_LOG_ERROR("Failed to flip image horizontally: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    return Error::None;
}

}   // namespace Jam