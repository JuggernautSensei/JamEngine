#include "pch.h"

#include "ImageExporter.h"

#include "Graphics.h"
#include <DirectXTexEXR.h>

namespace jam
{

bool ImageExporter::Export()
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(m_texh), "invalid texture handle");

    if (m_path.empty())
    {
        ErrorBuffer::Set("file path is empty");
        return false;
    }

    HRESULT               hr;
    const std::wstring    path       = m_path.wstring();
    const eImageFormat    format     = ImageFormatFrom(m_path);
    DirectX::ScratchImage scratchImg = Graphics::Capture(m_texh);

    switch (format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*scratchImg.GetImage(0, 0, 0), path.c_str());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(), DirectX::DDS_FLAGS_NONE, path.c_str());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*scratchImg.GetImage(0, 0, 0), path.c_str());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*scratchImg.GetImage(0, 0, 0), path.c_str());
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
            hr = DirectX::SaveToWICFile(*scratchImg.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(WICCodecsFrom_(format)), path.c_str());
            break;

        default:
            ErrorBuffer::Set("Unsupported image format: '{}'", m_path.native());
            return false;
    }

    if (FAILED(hr))
    {
        ErrorBuffer::Set("Failed to save image to file: '{}', {}", m_path.native(), ErrorMessageFrom(hr));
        return false;
    }

    return true;
}

void ImageExporter::SetPath(
    const fs::path& _path)
{
    m_path = _path;
}

void ImageExporter::SetTexture(
    const TextureHandle _texh)
{
    JAM_ASSERT(_texh != k_invalidHandle, "invalid texture handle");
    m_texh = _texh;
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
            JAM_ASSERT(0, "Unsupported image format: {}", ErrorMessageFrom(_format));
            return static_cast<DirectX::WICCodecs>(0);
    }
}

}   // namespace jam