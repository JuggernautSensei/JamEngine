#include "pch.h"

#include "ImageBase.h"

namespace Jam
{

Optional<eImageFormat> ParseAsImageFormat(
    const StringView _ext)
{
    if (_ext.IsEmpty())
    {
        return k_nullopt;
    }

    const StringView ext = _ext.StartsWith(".") ? _ext.SubView(1) : _ext;
    switch (ext.Hash32IgnoreCase())
    {
        case String::Hashed32IgnoreCase("dds"):
            return eImageFormat::DDS;
        case String::Hashed32IgnoreCase("exr"):
            return eImageFormat::EXR;
        case String::Hashed32IgnoreCase("hdr"):
            return eImageFormat::HDR;
        case String::Hashed32IgnoreCase("tga"):
            return eImageFormat::TGA;
        case String::Hashed32IgnoreCase("bmp"):
            return eImageFormat::BMP;
        case String::Hashed32IgnoreCase("jpg"):
            return eImageFormat::JPG;
        case String::Hashed32IgnoreCase("jpeg"):
            return eImageFormat::JPEG;
        case String::Hashed32IgnoreCase("png"):
            return eImageFormat::PNG;
        case String::Hashed32IgnoreCase("tif"):
            return eImageFormat::TIF;
        case String::Hashed32IgnoreCase("tiff"):
            return eImageFormat::TIFF;
        case String::Hashed32IgnoreCase("gif"):
            return eImageFormat::GIF;
        case String::Hashed32IgnoreCase("hdp"):
            return eImageFormat::HDP;
        case String::Hashed32IgnoreCase("jxr"):
            return eImageFormat::JXR;
        case String::Hashed32IgnoreCase("wdp"):
            return eImageFormat::WDP;
        case String::Hashed32IgnoreCase("ico"):
            return eImageFormat::ICO;
        case String::Hashed32IgnoreCase("heif"):
            return eImageFormat::HEIF;
        case String::Hashed32IgnoreCase("heic"):
            return eImageFormat::HEIC;
        default:
            return k_nullopt;
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

}   // namespace Jam    