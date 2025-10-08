#include "pch.h"

#include "ImageExporter.h"

#include "WindowsUtilities.h"

#include <DirectXTexEXR.h>
#include <SDL3/SDL.h>
using namespace jam;

namespace
{

DirectX::WICCodecs k_unknownWICCodecs = static_cast<DirectX::WICCodecs>(0);

DirectX::WICCodecs k_wicCodecsTable[EnumCount<eImageFormat>()] = {
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

}   // namespace

namespace jam
{

eErrorCode ImageExporter::Export()
{
    JAM_ASSERT(!m_filePath.empty(), "file path is empty");
    JAM_ASSERT(m_scratchImg.GetImageCount() > 0, "no image to export");
    JAM_ASSERT(IsExportablePath(m_filePath), "unsupported image format: '{}'", m_filepath.native());

    HRESULT      hr     = E_FAIL;
    std::wstring wPath  = m_filePath.wstring();
    eImageFormat format = DetermineFormatFromExtension(m_filePath);
    switch (format)
    {
        case eImageFormat::EXR:
            hr = DirectX::SaveToEXRFile(*m_scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::DDS:
            hr = DirectX::SaveToDDSFile(m_scratchImg.GetImages(), m_scratchImg.GetImageCount(), m_scratchImg.GetMetadata(), DirectX::DDS_FLAGS_NONE, wPath.c_str());
            break;

        case eImageFormat::HDR:
            hr = DirectX::SaveToHDRFile(*m_scratchImg.GetImage(0, 0, 0), wPath.c_str());
            break;

        case eImageFormat::TGA:
            hr = DirectX::SaveToTGAFile(*m_scratchImg.GetImage(0, 0, 0), wPath.c_str());
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
            hr = DirectX::SaveToWICFile(*m_scratchImg.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(k_wicCodecsTable[EnumToInt(format)]), wPath.c_str());
            break;

        default:
            SetErrorString("Unsupported image format: '{}'", m_filePath.native());
            return eErrorCode::UnsupportedFile;
    }

    if (FAILED(hr))
    {
        SetErrorString("Failed to save image to file: '{}'", m_filePath.native());
        JAM_LOG_DEBUG("HRESULT: {}", CreateComErrorMessage(hr));
        return eErrorCode::FileReadFailed;
    }

    return eErrorCode::Ok;
}

void ImageExporter::SetImage(DirectX::ScratchImage&& _image)
{
    m_scratchImg = std::move(_image);
}

void ImageExporter::SetPath(const fs::path& _filePath)
{
    m_filePath = _filePath;
    JAM_ASSERT(IsExportablePath(m_filePath), "unsupported image format: '{}'", _filepath.native());
}

void ImageExporter::Reset()
{
    m_scratchImg.Release();
    m_filePath.clear();
}

bool ImageExporter::IsExportablePath(const fs::path& _filePath) const
{
    eImageFormat format = DetermineFormatFromExtension(_filePath);
    return format != eImageFormat::Unknown;
}

}   // namespace jam