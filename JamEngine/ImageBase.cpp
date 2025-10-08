#include "pch.h"

#include "ImageBase.h"

#include "StringUtilities.h"

namespace jam
{

eImageFormat DetermineFormatFromExtension(const fs::path& _filePath)
{
    const fs::path& ext      = _filePath.extension();
    std::string     lowerExt = ToLower(ext.native());
    switch (HashOf(lowerExt))
    {
        case HashOf(".exr"):
            return eImageFormat::EXR;
        case HashOf(".dds"):
            return eImageFormat::DDS;
        case HashOf(".hdr"):
            return eImageFormat::HDR;
        case HashOf(".tga"):
            return eImageFormat::TGA;
        case HashOf(".bmp"):
            return eImageFormat::BMP;
        case HashOf(".jpg"):
            return eImageFormat::JPG;
        case HashOf(".jpeg"):
            return eImageFormat::JPEG;
        case HashOf(".tif"):
            return eImageFormat::TIF;
        case HashOf(".tiff"):
            return eImageFormat::TIFF;
        case HashOf(".gif"):
            return eImageFormat::GIF;
        case HashOf(".hdp"):
            return eImageFormat::HDP;
        case HashOf(".jxr"):
            return eImageFormat::JXR;
        case HashOf(".wdp"):
            return eImageFormat::WDP;
        case HashOf(".ico"):
            return eImageFormat::ICO;
        case HashOf(".heif"):
            return eImageFormat::HEIF;
        case HashOf(".heic"):
            return eImageFormat::HEIC;
        default:
            return eImageFormat::Unknown;
    }
}

}