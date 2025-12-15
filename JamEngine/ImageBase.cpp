#include "pch.h"

#include "ImageBase.h"

#include "StringUtilities.h"

namespace jam
{

[[nodiscard]] static eImageFormat ImageFormatFromLowerExt(const std::string_view _ext)
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
            return eImageFormat::Unknown;
    }
}

///////////////////////////////////////////////////////

eImageFormat ImageFormatFrom(
    const fs::path& _path)
{
    const std::string_view path = _path.native();
    if (path[0] == '.')   // already an extension
    {
        std::string ext = ToLower(path);
        return ImageFormatFromLowerExt(ext);
    }
    else
    {
        fs::path    ext      = _path.extension();
        std::string lowerExt = ToLower(ext.native());
        return ImageFormatFromLowerExt(lowerExt);
    }
}

std::string_view ErrorMessageFrom(
    const eImageFormat _format)
{
    switch (_format)
    {
        case eImageFormat::EXR:
            return "exr";
        case eImageFormat::DDS:
            return "dds";
        case eImageFormat::HDR:
            return "hdr";
        case eImageFormat::TGA:
            return "tga";
        case eImageFormat::BMP:
            return "bmp";
        case eImageFormat::JPG:
            return "jpg";
        case eImageFormat::JPEG:
            return "jpeg";
        case eImageFormat::PNG:
            return "png";
        case eImageFormat::TIF:
            return "tif";
        case eImageFormat::TIFF:
            return "tiff";
        case eImageFormat::GIF:
            return "gif";
        case eImageFormat::HDP:
            return "hdp";
        case eImageFormat::JXR:
            return "jxr";
        case eImageFormat::WDP:
            return "wdp";
        case eImageFormat::ICO:
            return "ico";
        case eImageFormat::HEIF:
            return "heif";
        case eImageFormat::HEIC:
            return "heic";
        default:
            JAM_ASSERT(0, "Unknown image format");
            return "Unknown";
    }
}

}   // namespace jam