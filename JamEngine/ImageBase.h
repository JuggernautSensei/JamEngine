#pragma once

namespace jam
{

enum class eImageFormat : Int8
{
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

[[nodiscard]] eImageFormat     ImageFormatFrom(const fs::path& _path);
[[nodiscard]] std::string_view ErrorMessageFrom(eImageFormat _format);

}   // namespace jam