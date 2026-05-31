#include "pch.h"
#include "ImageExporter.h"

#include <DirectXTexEXR.h>

#include "FileSystem.h"
#include "Graphics.h"
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Error ImageExporter::ExportToFile(
    const ImageContainer&            _container,
    const StringView                 _path,
    const eImageFormat               _format,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy)
{
    JAM_ASSERT(_policy != eFileWriterCollisionPolicy::Append, "WriteElement policy is not supported for image export.");

    StringView dir = {}, base = {}, ext = {};
    _path.Decompose(&dir, nullptr, &base, &ext);

    WString path = WString::Encoded(dir);
    path.AppendEncode(base);
    path.AppendEncode(_bReplaceExtension ? ExtensionOf(_format) : ext);

    if (_policy != eFileWriterCollisionPolicy::Overwrite && FileSystem::Utf16::Exists(path))
    {
        switch (_policy)
        {
            case eFileWriterCollisionPolicy::Rename:
                path = FileSystem::Utf16::GenerateUniquePath(path);
                break;

            case eFileWriterCollisionPolicy::Fail:
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
            hr = DirectX::SaveToEXRFile(*_container.GetImage(0, 0, 0), path.GetPtr());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(_container.GetImages(), _container.GetImageCount(), _container.GetMetadata(), DirectX::DDS_FLAGS_NONE, path.GetPtr());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*_container.GetImage(0, 0, 0), path.GetPtr());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*_container.GetImage(0, 0, 0), path.GetPtr());
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
            hr = DirectX::SaveToWICFile(*_container.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), path.GetPtr());
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

Error ImageExporter::ExportToFile(
    const TextureHandle              _texh,
    const StringView                 _path,
    const eImageFormat               _format,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy)
{
    Graphics& gfx = GetGraphics();
    return ExportToFile(gfx.Capture(_texh), _path, _format, _bReplaceExtension, _policy);
}

Expect<ImageBlob> ImageExporter::ExportToMemory(
    const ImageContainer& _container,
    const eImageFormat    _format)
{
    ImageBlob blob = {};
    HRESULT   hr   = S_OK;
    switch (_format)
    {
        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSMemory(_container.GetImages(), _container.GetImageCount(), _container.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRMemory(*_container.GetImage(0, 0, 0), blob);
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAMemory(*_container.GetImage(0, 0, 0), DirectX::TGA_FLAGS_NONE, blob);
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
            hr = DirectX::SaveToWICMemory(_container.GetImages(), _container.GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), blob);
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

    return Error::None;
}

Expect<ImageBlob> ImageExporter::ExportToMemory(
    const TextureHandle _texh,
    const eImageFormat  _format)
{
    Graphics& gfx = GetGraphics();
    return ExportToMemory(gfx.Capture(_texh), _format);
}

}   // namespace Jam