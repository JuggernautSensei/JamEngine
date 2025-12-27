#include "pch.h"

#include "ImageExporter.h"

#include "Graphics.h"
#include <DirectXTexEXR.h>

#include "Profile.h"

namespace jam
{

bool ImageExporter::Export(
    const TextureHandle _texh,
    const fs::path&     _path,
    const eImageFormat  _format,
    const bool          _bChangeExtenstion) const
{
    JAM_FUNCTION_MSG_PROFILE("Export Image: {}", _path.native());
    JAM_ASSERT(_texh != k_invalidHandle, "Invalid texture handle");

    fs::path exportPath = _path;
    if (_bChangeExtenstion)
    {
        const StringView ext = StringFrom(_format);
        exportPath.replace_extension(ext.GetPtr());
    }

    const DirectX::ScratchImage scratchImg = Graphics::Capture(_texh);
    const std::wstring          wPath      = exportPath.wstring();
    HRESULT                     hr;
    switch (_format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(), DirectX::DDS_FLAGS_NONE, wPath.c_str());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*scratchImg.GetImage(0, 0, 0), wPath.c_str());
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
            hr = DirectX::SaveToWICFile(*scratchImg.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(_format)), wPath.c_str());
            break;

        default:
            GlobalBuffer::SetLastError("Unsupported image format: '{}'", exportPath.native());
            return false;
    }
    if (FAILED(hr))
    {
        GlobalBuffer::SetLastError("Failed to save image to file: '{}', {}", exportPath.native(), StringFrom(hr));
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
            JAM_ASSERT(0, "Unsupported image format: {}", StringFrom(_format));
            return static_cast<DirectX::WICCodecs>(0);
    }
}

}   // namespace jam