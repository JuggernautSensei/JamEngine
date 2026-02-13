#include "pch.h"
#include "ImageExporter.h"

#include <DirectXTexEXR.h>

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
    const ImageContainer&       _container,
    const Path&                 _path,
    const eImageFormat          _format,
    const bool                  _bReplaceExtension,
    const Flags<eSaveFileFlags> _flags)
{
    if (const Error err = ResolveSaveFileFlags(_path, _flags))
    {
        return err;
    }

    ScratchWBuffer pathBuf;
    if (_bReplaceExtension)
    {
        const Path path = ReplacedExtension(_path, ExtensionOf(_format));
        pathBuf.SetUtf8(path.native());
    }
    else
    {
        pathBuf.SetUtf8(_path.native());
    }

    HRESULT hr = S_OK;
    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*_container.GetImage(0, 0, 0), pathBuf.GetPtr());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(_container.GetImages(), _container.GetImageCount(), _container.GetMetadata(), DirectX::DDS_FLAGS_NONE, pathBuf.GetPtr());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*_container.GetImage(0, 0, 0), pathBuf.GetPtr());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*_container.GetImage(0, 0, 0), pathBuf.GetPtr());
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
            hr = DirectX::SaveToWICFile(*_container.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), pathBuf.GetPtr());
            break;

        default:
            JAM_LOG.Error("Unsupported image format: '{}'", _path.native());
            return Error::UnsupportedFormat;
    }

    if (FAILED(hr))
    {
        ScratchBuffer buf;
        FormatHRESULTMessage(buf, hr);

        JAM_LOG.Error("Failed to save image to file: '{}', {}", _path, buf.ToStringView());
        return Error::ExportFailed;
    }

    return Error::None;
}

Error ImageExporter::ExportToFile(
    const TextureHandle         _texh,
    const Path&                 _path,
    const eImageFormat          _format,
    const bool                  _bReplaceExtension,
    const Flags<eSaveFileFlags> _flags)
{
    return ExportToFile(JAM_GFX.Capture(_texh), _path, _format, _bReplaceExtension, _flags);
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
    }

    if (FAILED(hr))
    {
        ScratchBuffer buf;
        FormatHRESULTMessage(buf, hr);

        JAM_LOG.Error("Failed to export image to memory: {}", buf.ToStringView());
        return Error::ExportFailed;
    }

    return Error::None;
}

Expect<ImageBlob> ImageExporter::ExportToMemory(
    const TextureHandle _texh,
    const eImageFormat  _format)
{
    return ExportToMemory(JAM_GFX.Capture(_texh), _format);
}

}   // namespace Jam