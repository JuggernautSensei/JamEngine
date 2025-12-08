#pragma once
#include <DirectXTex.h>

namespace jam
{

enum class eImageFormat : Int8
{
    Unknown = -1,

    DDS = 0,
    EXR,
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

[[nodiscard]] eImageFormat     ImageFormatFromPath(const fs::path& _path);
[[nodiscard]] eImageFormat     ImageFormatFromString(std::string_view _str);
[[nodiscard]] std::string_view ImageFormatToString(eImageFormat _format);

}   // namespace jam