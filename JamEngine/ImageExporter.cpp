#include "pch.h"

#include "ImageExporter.h"

#include "Graphics.h"
#include <DirectXTexEXR.h>

#include "Profile.h"
#include "WindowsUtilities.h"

namespace jam
{

bool ImageExporter::Export(
    const TextureHandle  _texh,
    const fs::path&      _path,
    const eImageFormat   _format,
    const bool           _bReplaceExtension,
    const eSaveFileFlags _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("Export Image: {}", _path.native());
    JAM_ASSERT(_texh != k_invalidHandle, "Invalid texture handle");

    if (!CheckSaveFileFlags(_path, _flags))
    {
        return false;
    }

    // capture texture to scratch image
    DirectX::ScratchImage scratchImg = Graphics::Capture(_texh);
    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(ExtensionOf(_format).GetPtr());
        return Export_(scratchImg, path, _format);
    }

    return Export_(scratchImg, _path, _format);
}

bool ImageExporter::Export(
    const DirectX::ScratchImage& _scratchImg,
    const fs::path&              _path,
    const eImageFormat           _format,
    const bool                   _bReplaceExtension,
    const eSaveFileFlags         _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("Export Image: {}", _path.native());

    if (!CheckSaveFileFlags(_path, _flags))
    {
        return false;
    }

    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(ExtensionOf(_format).GetPtr());
        return Export_(_scratchImg, path, _format);
    }

    return Export_(_scratchImg, _path, _format);
}

bool ImageExporter::Export_(
    const DirectX::ScratchImage& _scratchImg,
    const fs::path&              _path,
    const eImageFormat           _format) const
{
    const std::wstring wPath = _path.wstring();
    HRESULT            hr    = S_OK;
    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*_scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(_scratchImg.GetImages(), _scratchImg.GetImageCount(), _scratchImg.GetMetadata(), DirectX::DDS_FLAGS_NONE, wPath.c_str());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*_scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*_scratchImg.GetImage(0, 0, 0), wPath.c_str());
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
            hr = DirectX::SaveToWICFile(*_scratchImg.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), wPath.c_str());
            break;

        default:
            GlobalBuffer::SetLastError("Unsupported image format: '{}'", _path.native());
            return false;
    }

    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to save image to file: '{}', {}", _path.native(), CreateErrorMessageFromHRESULT(hr));
        return false;
    }

    return true;
}

DirectX::WICCodecs ImageExporter::WICCodecsFrom_(
    const eImageFormat _format) const
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
            JAM_ASSERT(0, "Unsupported image format: {}", ExtensionOf(_format));
            return static_cast<DirectX::WICCodecs>(0);
    }
}

}   // namespace jam