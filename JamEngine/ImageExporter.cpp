#include "pch.h"

#include "ImageExporter.h"

#include "Graphics.h"
#include <DirectXTexEXR.h>

namespace jam
{

constexpr DirectX::WICCodecs k_unknownWICCodecs = static_cast<DirectX::WICCodecs>(0);

constexpr DirectX::WICCodecs k_wicCodecsTable[EnumCount<eImageFormat>()] = {
    k_unknownWICCodecs,        // EXR
    k_unknownWICCodecs,        // DDS
    k_unknownWICCodecs,        // HDR
    k_unknownWICCodecs,        // TGA
    DirectX::WIC_CODEC_BMP,    // BMP
    DirectX::WIC_CODEC_JPEG,   // JPG
    DirectX::WIC_CODEC_JPEG,   // JPEG
    DirectX::WIC_CODEC_PNG,    // PNG
    DirectX::WIC_CODEC_TIFF,   // TIF
    DirectX::WIC_CODEC_TIFF,   // TIFF
    DirectX::WIC_CODEC_GIF,    // GIF
    DirectX::WIC_CODEC_WMP,    // HDP
    DirectX::WIC_CODEC_WMP,    // JXR
    DirectX::WIC_CODEC_WMP,    // WDP
    DirectX::WIC_CODEC_ICO,    // ICO
    DirectX::WIC_CODEC_HEIF,   // HEIF
    DirectX::WIC_CODEC_HEIF,   // HEIC
};

bool ImageExporter::Export()
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(m_texh), "invalid texture handle");

    if (m_path.empty())
    {
        SetErrorString("file path is empty");
        return false;
    }

    HRESULT               hr;
    const std::wstring    wPath      = m_path.wstring();
    const eImageFormat    format     = ImageFormatFromPath(m_path);
    DirectX::ScratchImage scratchImg = Graphics::Capture(m_texh);

    switch (format)
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
            hr = DirectX::SaveToWICFile(*scratchImg.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(k_wicCodecsTable[EnumToInt(format)]), wPath.c_str());
            break;

        default:
            SetErrorString("Unsupported image format: '{}'", m_path.native());
            return false;
    }

    if (FAILED(hr))
    {
        SetErrorString("Failed to save image to file: '{}', {}", m_path.native(), CreateComErrorString(hr));
        return false;
    }

    return true;
}

void ImageExporter::SetPath(const fs::path& _path)
{
    m_path = _path;
}

void ImageExporter::SetTexture(const TextureHandle _texh)
{
    JAM_ASSERT(_texh != k_invalidHandle, "invalid texture handle");
    m_texh = _texh;
}

}   // namespace jam