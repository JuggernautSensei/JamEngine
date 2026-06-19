#include "pch.h"

#include "Image.h"

#include <DirectXTexEXR.h>

#include "FileSystem.h"
#include "WindowsUtilities.h"

namespace Jam
{

namespace
{

    [[nodiscard]] DirectX::WICCodecs WICCodecsFrom_(
        const eImageFormat _format)
    {
        switch (_format)
        {
            case eImageFormat::BMP:
                return DirectX::WIC_CODEC_BMP;

            case eImageFormat::JPG:
            case eImageFormat::JPEG:
                return DirectX::WIC_CODEC_JPEG;

            case eImageFormat::PNG:
                return DirectX::WIC_CODEC_PNG;

            case eImageFormat::TIF:
            case eImageFormat::TIFF:
                return DirectX::WIC_CODEC_TIFF;

            case eImageFormat::GIF:
                return DirectX::WIC_CODEC_GIF;

            case eImageFormat::HDP:
            case eImageFormat::JXR:
            case eImageFormat::WDP:
                return DirectX::WIC_CODEC_WMP;

            case eImageFormat::ICO:
                return DirectX::WIC_CODEC_ICO;

            case eImageFormat::HEIF:
            case eImageFormat::HEIC:
                return DirectX::WIC_CODEC_HEIF;

            default:
                JAM_UNREACHABLE;
        }
    }

}   // namespace

Optional<eImageFormat> ParseAsImageFormat(
    const StringView _ext)
{
    if (_ext.IsEmpty())
    {
        return kNullopt;
    }

    const StringView ext = _ext.StartsWith(".") ? _ext.SubView(1) : _ext;
    switch (ext.IgnoreCaseHash())
    {
        case IgnoreCaseHashOf("dds"):
            return eImageFormat::DDS;
        case IgnoreCaseHashOf("exr"):
            return eImageFormat::EXR;
        case IgnoreCaseHashOf("hdr"):
            return eImageFormat::HDR;
        case IgnoreCaseHashOf("tga"):
            return eImageFormat::TGA;
        case IgnoreCaseHashOf("bmp"):
            return eImageFormat::BMP;
        case IgnoreCaseHashOf("jpg"):
            return eImageFormat::JPG;
        case IgnoreCaseHashOf("jpeg"):
            return eImageFormat::JPEG;
        case IgnoreCaseHashOf("png"):
            return eImageFormat::PNG;
        case IgnoreCaseHashOf("tif"):
            return eImageFormat::TIF;
        case IgnoreCaseHashOf("tiff"):
            return eImageFormat::TIFF;
        case IgnoreCaseHashOf("gif"):
            return eImageFormat::GIF;
        case IgnoreCaseHashOf("hdp"):
            return eImageFormat::HDP;
        case IgnoreCaseHashOf("jxr"):
            return eImageFormat::JXR;
        case IgnoreCaseHashOf("wdp"):
            return eImageFormat::WDP;
        case IgnoreCaseHashOf("ico"):
            return eImageFormat::ICO;
        case IgnoreCaseHashOf("heif"):
            return eImageFormat::HEIF;
        case IgnoreCaseHashOf("heic"):
            return eImageFormat::HEIC;
        default:
            return kNullopt;
    }
}

StringView ExtensionOf(
    const eImageFormat _format)
{
    switch (_format)
    {
        case eImageFormat::DDS:
            return ".dds";
        case eImageFormat::EXR:
            return ".exr";
        case eImageFormat::HDR:
            return ".hdr";
        case eImageFormat::TGA:
            return ".tga";
        case eImageFormat::BMP:
            return ".bmp";
        case eImageFormat::JPG:
            return ".jpg";
        case eImageFormat::JPEG:
            return ".jpeg";
        case eImageFormat::PNG:
            return ".png";
        case eImageFormat::TIF:
            return ".tif";
        case eImageFormat::TIFF:
            return ".tiff";
        case eImageFormat::GIF:
            return ".gif";
        case eImageFormat::HDP:
            return ".hdp";
        case eImageFormat::JXR:
            return ".jxr";
        case eImageFormat::WDP:
            return ".wdp";
        case eImageFormat::ICO:
            return ".ico";
        case eImageFormat::HEIF:
            return ".heif";
        case eImageFormat::HEIC:
            return ".heic";
        default:
            JAM_UNREACHABLE;
    }
}

Image::Image(ImageContainer&& _texh)
    : m_container(std::move(_texh))
{
}

Expect<Image> Image::ImportFromFile(
    const StringView   _path,
    const eImageFormat _format)
{
    Image image = {};
    if (const Error err = image.ImportFromFile_(_path, _format))
    {
        return err;
    }
    return image;
}

Expect<Image> Image::ImportFromFile(
    const StringView _path)
{
    Image image = {};
    if (const Error err = image.ImportFromFile_(_path))
    {
        return err;
    }
    return image;
}

Expect<Image> Image::ImportFromMemory(
    const MemoryView   _mem,
    const eImageFormat _format)
{
    Image image = {};
    if (const Error err = image.ImportFromMemory_(_mem, _format))
    {
        return err;
    }
    return image;
}

const ImageContainer& Image::GetImageContainer() const
{
    return m_container;
}

Error Image::ExportToFile(
    StringView         _path,
    const eImageFormat _format,
    const eFileWrite   _mode) const
{
    JAM_ASSERT(_mode != eFileWrite::Append, "WriteElement policy is not supported for image export.");

    WString path = WString::Encoded(_path);
    if (_mode != eFileWrite::Truncate && FS::Utf16::Exists(path))
    {
        switch (_mode)
        {
            case eFileWrite::Rename:
                path = FS::Utf16::MakeUniquePath(path);
                break;

            case eFileWrite::FailIfExists:
                JAM_LOG_ERROR(L"File already exists: '{}'", path);
                return Error::AlreadyExists;

            default:
                JAM_UNREACHABLE;
        }
    }

    HRESULT hr = S_OK;
    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*m_container.GetImage(0, 0, 0), path.GetPtr());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(m_container.GetImages(), m_container.GetImageCount(), m_container.GetMetadata(), DirectX::DDS_FLAGS_NONE, path.GetPtr());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*m_container.GetImage(0, 0, 0), path.GetPtr());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*m_container.GetImage(0, 0, 0), path.GetPtr());
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
            hr = DirectX::SaveToWICFile(*m_container.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), path.GetPtr());
            break;

        default:
            JAM_LOG_ERROR("Unsupported image format: '{}'", _path);
            return Error::UnsupportedFormat;
    }

    if (FAILED(hr))
    {
        JAM_LOG_ERROR("Failed to save image to file: '{}', {}", _path, MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        return Error::ExportFailed;
    }

    return Error::None;
}

Expect<ImageBlob> Image::ExportToMemory(const eImageFormat _format) const
{
    ImageBlob blob = {};
    HRESULT   hr   = S_OK;
    switch (_format)
    {
        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSMemory(m_container.GetImages(), m_container.GetImageCount(), m_container.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRMemory(*m_container.GetImage(0, 0, 0), blob);
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAMemory(*m_container.GetImage(0, 0, 0), DirectX::TGA_FLAGS_NONE, blob);
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
            hr = DirectX::SaveToWICMemory(m_container.GetImages(), m_container.GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), blob);
            break;

        default:
            JAM_LOG_ERROR("Unsupported image format for memory export: '{}'", NameOf(_format));
            return Error::UnsupportedFormat;
    }

    if (FAILED(hr))
    {
        JAM_LOG_ERROR("Failed to export image to memory: {}", MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        return Error::ExportFailed;
    }

    return blob;
}

Error Image::ImportFromFile_(
    const StringView   _path,
    const eImageFormat _format)
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

    return Error::None;
}

Error Image::ImportFromFile_(const StringView _path)
{
    Optional<eImageFormat> format = ParseAsImageFormat(_path.GetExtension());
    if (!format)
    {
        JAM_LOG_ERROR("Unsupported image format for file import: '{}'", _path.GetExtension());
        return Error::UnsupportedFormat;
    }

    return ImportFromFile_(_path, format.GetValue());
}

Error Image::ImportFromMemory_(
    const MemoryView   _mem,
    const eImageFormat _format)
{
    // load from memory
    const Byte* pMem      = _mem.GetPtr();
    const SizeT byteWidth = _mem.GetByteWidth();
    HRESULT     hr        = S_OK;

    switch (_format)
    {
        case eImageFormat::DDS:
            hr = DirectX::LoadFromDDSMemory(pMem, byteWidth, DirectX::DDS_FLAGS_NONE, nullptr, m_container);
            break;

        case eImageFormat::HDR:
            hr = DirectX::LoadFromHDRMemory(pMem, byteWidth, nullptr, m_container);
            break;

        case eImageFormat::TGA:
            hr = DirectX::LoadFromTGAMemory(pMem, byteWidth, DirectX::TGA_FLAGS_NONE, nullptr, m_container);
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
            hr = DirectX::LoadFromWICMemory(pMem, byteWidth, DirectX::WIC_FLAGS_NONE, nullptr, m_container);
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

    return Error::None;
}

}   // namespace Jam