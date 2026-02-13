#include "pch.h"
#include "ImageImporter.h"

#include <DirectXTexEXR.h>

#include "WindowsUtilities.h"

namespace Jam
{

Expect<ImageImporter> ImageImporter::ImportFromFile(
    const Path&               _path,
    const eImageFormat        _format,
    const ImageImportOptions& _options)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _format, _options))
    {
        return err;
    }

    return importer;
}

Expect<ImageImporter> ImageImporter::ImportFromFile(
    const Path&               _path,
    const ImageImportOptions& _options)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _options))
    {
        return err;
    }

    return importer;
}

Expect<ImageImporter> ImageImporter::ImportFromMemory(
    const CMemoryBlock        _mem,
    const eImageFormat        _format,
    const ImageImportOptions& _options)
{
    ImageImporter importer = {};
    if (const Error err = importer.ImportFromMemory_(_mem, _format, _options))
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
    const Path&               _path,
    const eImageFormat        _format,
    const ImageImportOptions& _options)
{
    // load from file
    ImageContainer     container = {};
    const std::wstring wPath     = _path.wstring();
    HRESULT            hr        = S_OK;

    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::LoadFromEXRFile(wPath.c_str(), nullptr, container);
            break;

        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, container);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRFile(wPath.c_str(), nullptr, container);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAFile(wPath.c_str(), nullptr, container);
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
            hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, container);
            break;

        default:
            JAM_UNREACHABLE;
    }

    if (FAILED(hr))
    {
        ScratchBuffer buf;
        FormatHRESULTMessage(buf, hr);
        JAM_LOG.Error("Failed to load image file '{}': {}", _path.native(), buf.ToStringView());
        return Error::ImportFailed;
    }

    // post process
    return PostProcess_(_options);
}

Error ImageImporter::ImportFromFile_(
    const Path&               _path,
    const ImageImportOptions& _options)
{
    Optional<eImageFormat> format = ParseAsImageFormat(_path);
    if (!format)
    {
        JAM_LOG.Error("Unsupported image format for file import: '{}'", _path.extension().string());
        return Error::UnsupportedFormat;
    }

    return ImportFromFile_(_path, format.GetValue(), _options);
}

Error ImageImporter::ImportFromMemory_(
    const CMemoryBlock        _mem,
    const eImageFormat        _format,
    const ImageImportOptions& _options)
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
            JAM_LOG.Error("Unsupported image format for memory import: '{}'", ExtensionOf(_format));
            return Error::UnsupportedFormat;
    }

    if (FAILED(hr))
    {
        ScratchBuffer buf;
        FormatHRESULTMessage(buf, hr);

        JAM_LOG.Error("Failed to load image from memory: {}", buf.ToStringView());
        return Error::ImportFailed;
    }

    // post process
    return PostProcess_(_options);
}

Error ImageImporter::PostProcess_(
    const ImageImportOptions& _options)
{
    // convert format
    if (_options.bConvertFormat)
    {
        const TextureFormatInfo& texInfo = TextureFormatInfoFrom(_options.convertFormat);
        if (texInfo.texFormat == DXGI_FORMAT_UNKNOWN)
        {
            JAM_LOG.Error("invalid format to convert: '{}'", NameOf(_options.convertFormat));
            return Error::InvalidArgument;
        }

        ImageContainer result = {};
        const HRESULT  hr     = DirectX::Convert(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            texInfo.texFormat,
            DirectX::TEX_FILTER_DEFAULT,
            _options.convertThreshold,
            result);

        if (FAILED(hr))
        {
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);

            JAM_LOG.Error("Failed to convert image format: {}", buf.ToStringView());
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // force sRGB
    if (_options.bForceSRGB)
    {
        const DirectX::TexMetadata& metadata = m_container.GetMetadata();
        if (!DirectX::IsSRGB(metadata.format))
        {
            const DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);
            if (srgbFormat == DXGI_FORMAT_UNKNOWN)
            {
                JAM_LOG.Error("Failed to convert image format to sRGB");
                return Error::InvalidArgument;
            }

            ImageContainer result = {};
            const HRESULT  hr     = DirectX::Convert(
                m_container.GetImages(),
                m_container.GetImageCount(),
                m_container.GetMetadata(),
                srgbFormat,
                DirectX::TEX_FILTER_DEFAULT,
                _options.convertThreshold,
                result);

            if (FAILED(hr))
            {
                ScratchBuffer buf;
                FormatHRESULTMessage(buf, hr);

                JAM_LOG.Error("Failed to convert to sRGB: {}", buf.ToStringView());
                return Error::RuntimeError;
            }

            m_container = std::move(result);
        }
    }

    // premultiply alpha
    if (_options.bPreMultiplyAlpha)
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
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);

            JAM_LOG.Error("Failed to premultiply alpha: {}", buf.ToStringView());
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // resize image
    if (_options.bResize)
    {
        JAM_ASSERT(_options.newWidth > 0 && _options.newHeight > 0, "new width and height must be greater than 0");
        JAM_ASSERT(m_container.GetMetadata().dimension != DirectX::TEX_DIMENSION_TEXTURE3D, "3D texture resizing is not supported");

        ImageContainer result = {};
        const HRESULT  hr     = DirectX::Resize(
            m_container.GetImages(),
            m_container.GetImageCount(),
            m_container.GetMetadata(),
            _options.newWidth,
            _options.newHeight,
            DirectX::TEX_FILTER_DEFAULT,
            result);

        if (FAILED(hr))
        {
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);

            JAM_LOG.Error("Failed to resize image to {}x{}: {}", _options.newWidth, _options.newHeight, buf.ToStringView());
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // drop or generate mips
    if (_options.bGenerateMips)
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
                ScratchBuffer buf;
                FormatHRESULTMessage(buf, hr);

                JAM_LOG.Error("Failed to generate mipmaps: {}", buf.ToStringView());
                return Error::RuntimeError;
            }

            m_container = std::move(result);
        }
    }

    // flip vertical
    if (_options.bFlipVertical)
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
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);

            JAM_LOG.Error("Failed to flip image vertically: {}", buf.ToStringView());
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    // flip horizontal
    if (_options.bFlipHorizontal)
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
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);

            JAM_LOG.Error("Failed to flip image horizontally: {}", buf.ToStringView());
            return Error::RuntimeError;
        }

        m_container = std::move(result);
    }

    return Error::None;
}

}   // namespace Jam