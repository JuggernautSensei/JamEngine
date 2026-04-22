#include "pch.h"

#include "ImageBase.h"

namespace Jam
{

Optional<eImageFormat> ParseAsImageFormat(
    const StringView _ext)
{
    if (_ext.IsEmpty())
    {
        return kNullopt;
    }

    const StringView ext = _ext.StartsWith(".") ? _ext.SubView(1) : _ext;
    switch (ext.IgnoreCaseHash32())
    {
        case String::IgnoreCaseHashed32("dds"):
            return eImageFormat::DDS;
        case String::IgnoreCaseHashed32("exr"):
            return eImageFormat::EXR;
        case String::IgnoreCaseHashed32("hdr"):
            return eImageFormat::HDR;
        case String::IgnoreCaseHashed32("tga"):
            return eImageFormat::TGA;
        case String::IgnoreCaseHashed32("bmp"):
            return eImageFormat::BMP;
        case String::IgnoreCaseHashed32("jpg"):
            return eImageFormat::JPG;
        case String::IgnoreCaseHashed32("jpeg"):
            return eImageFormat::JPEG;
        case String::IgnoreCaseHashed32("png"):
            return eImageFormat::PNG;
        case String::IgnoreCaseHashed32("tif"):
            return eImageFormat::TIF;
        case String::IgnoreCaseHashed32("tiff"):
            return eImageFormat::TIFF;
        case String::IgnoreCaseHashed32("gif"):
            return eImageFormat::GIF;
        case String::IgnoreCaseHashed32("hdp"):
            return eImageFormat::HDP;
        case String::IgnoreCaseHashed32("jxr"):
            return eImageFormat::JXR;
        case String::IgnoreCaseHashed32("wdp"):
            return eImageFormat::WDP;
        case String::IgnoreCaseHashed32("ico"):
            return eImageFormat::ICO;
        case String::IgnoreCaseHashed32("heif"):
            return eImageFormat::HEIF;
        case String::IgnoreCaseHashed32("heic"):
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

}   // namespace Jam    