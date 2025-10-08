#pragma once
#include <DirectXTex.h>

namespace jam
{

enum class eImageFormat : Int8
{
    Unknown = -1,

    EXR = 0,
    DDS,
    HDR,
    TGA,

    BMP,
    JPG,
    JPEG,
    PNG,
    TIF,
    TIFF,
    GIF,
    HDP,
    JXR,
    WDP,
    ICO,
    HEIF,
    HEIC
};

JAM_NODISCARD eImageFormat DetermineFormatFromExtension(const fs::path& _filePath);

}   // namespace jam