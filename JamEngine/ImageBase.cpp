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

}   // namespace Jam    