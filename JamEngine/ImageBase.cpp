#include "pch.h"

#include "ImageBase.h"

#include "StringUtilities.h"

namespace jam
{

[[nodiscard]] Optional<eImageFormat> ImageFormatFromLowerExt(
    const StringView _ext)
{
    switch (HashOf(_ext))
    {
        case HashOf(".exr"):
        case HashOf("exr"):
            return eImageFormat::EXR;
        case HashOf(".dds"):
        case HashOf("dds"):
            return eImageFormat::DDS;
        case HashOf(".hdr"):
        case HashOf("hdr"):
            return eImageFormat::HDR;
        case HashOf(".tga"):
        case HashOf("tga"):
            return eImageFormat::TGA;
        case HashOf(".bmp"):
        case HashOf("bmp"):
            return eImageFormat::BMP;
        case HashOf(".jpg"):
        case HashOf("jpg"):
            return eImageFormat::JPG;
        case HashOf(".jpeg"):
        case HashOf("jpeg"):
            return eImageFormat::JPEG;
        case HashOf(".png"):
        case HashOf("png"):
            return eImageFormat::PNG;
        case HashOf(".tif"):
        case HashOf("tif"):
            return eImageFormat::TIF;
        case HashOf(".tiff"):
        case HashOf("tiff"):
            return eImageFormat::TIFF;
        case HashOf(".gif"):
        case HashOf("gif"):
            return eImageFormat::GIF;
        case HashOf(".hdp"):
        case HashOf("hdp"):
            return eImageFormat::HDP;
        case HashOf(".jxr"):
        case HashOf("jxr"):
            return eImageFormat::JXR;
        case HashOf(".wdp"):
        case HashOf("wdp"):
            return eImageFormat::WDP;
        case HashOf(".ico"):
        case HashOf("ico"):
            return eImageFormat::ICO;
        case HashOf(".heif"):
        case HashOf("heif"):
            return eImageFormat::HEIF;
        case HashOf(".heic"):
        case HashOf("heic"):
            return eImageFormat::HEIC;
        default:
            return k_nullopt;
    }
}

///////////////////////////////////////////////////////

Optional<eImageFormat> ImageFormatFrom(
    const fs::path& _path)
{
    const StringView path = _path.native();

    Int32 idx = 0;
    if (!MoveIfHasValue(path.FindLastOf('.'), idx))
    {
        return k_nullopt;
    }

    const StringView ext = path.SubView(idx);
    return ImageFormatFrom(ext);
}

Optional<eImageFormat> ImageFormatFrom(const StringView _format)
{
    const std::string lower = ToLower(_format);
    return ImageFormatFromLowerExt(lower);
}

StringView ExtensionOf(
    const eImageFormat _format,
    const bool         _bIncludeDot)
{
    StringView str;

    switch (_format)
    {
        case eImageFormat::EXR:
            str = ".exr";
            break;
        case eImageFormat::DDS:
            str = ".dds";
            break;
        case eImageFormat::HDR:
            str = ".hdr";
            break;
        case eImageFormat::TGA:
            str = ".tga";
            break;
        case eImageFormat::BMP:
            str = ".bmp";
            break;
        case eImageFormat::JPG:
            str = ".jpg";
            break;
        case eImageFormat::JPEG:
            str = ".jpeg";
            break;
        case eImageFormat::PNG:
            str = ".png";
            break;
        case eImageFormat::TIF:
            str = ".tif";
            break;
        case eImageFormat::TIFF:
            str = ".tiff";
            break;
        case eImageFormat::GIF:
            str = ".gif";
            break;
        case eImageFormat::HDP:
            str = ".hdp";
            break;
        case eImageFormat::JXR:
            str = ".jxr";
            break;
        case eImageFormat::WDP:
            str = ".wdp";
            break;
        case eImageFormat::ICO:
            str = ".ico";
            break;
        case eImageFormat::HEIF:
            str = ".heif";
            break;
        case eImageFormat::HEIC:
            str = ".heic";
            break;
        default:
            JAM_ASSERT(0, "Unknown image format");
            str = "unknown";
    }

    if (!_bIncludeDot)
    {
        str = str.SubView(1);
    }

    return str;
}

}   // namespace jam