#include "pch.h"

#include "ImageBase.h"

namespace Jam
{

Optional<eImageFormat> ParseAsImageFormat(
    const Path& _path)
{
    const StringView ext = ExtensionViewOf(_path);
    return ParseAsImageFormat(ext);
}

Optional<eImageFormat> ParseAsImageFormat(
    const StringView _ext)
{
    const StringView ext  = _ext.StartsWith(".") ? _ext.SubView(1) : _ext;
    const UInt32     hash = ext.Hash32IgnoreCase();

    switch (hash)
    {
        case HashString32IgnoreCase("dds"):
            return eImageFormat::DDS;
        case HashString32IgnoreCase("exr"):
            return eImageFormat::EXR;
        case HashString32IgnoreCase("hdr"):
            return eImageFormat::HDR;
        case HashString32IgnoreCase("tga"):
            return eImageFormat::TGA;
        case HashString32IgnoreCase("bmp"):
            return eImageFormat::BMP;
        case HashString32IgnoreCase("jpg"):
            return eImageFormat::JPG;
        case HashString32IgnoreCase("jpeg"):
            return eImageFormat::JPEG;
        case HashString32IgnoreCase("png"):
            return eImageFormat::PNG;
        case HashString32IgnoreCase("tif"):
            return eImageFormat::TIF;
        case HashString32IgnoreCase("tiff"):
            return eImageFormat::TIFF;
        case HashString32IgnoreCase("gif"):
            return eImageFormat::GIF;
        case HashString32IgnoreCase("hdp"):
            return eImageFormat::HDP;
        case HashString32IgnoreCase("jxr"):
            return eImageFormat::JXR;
        case HashString32IgnoreCase("wdp"):
            return eImageFormat::WDP;
        case HashString32IgnoreCase("ico"):
            return eImageFormat::ICO;
        case HashString32IgnoreCase("heif"):
            return eImageFormat::HEIF;
        case HashString32IgnoreCase("heic"):
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