#pragma once

namespace jam
{

enum class eImageFormat
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

// extract image format from _path or string
[[nodiscard]] Optional<eImageFormat> ImageFormatFrom(const fs::path& _path);
[[nodiscard]] Optional<eImageFormat> ImageFormatFrom(StringView _format);
[[nodiscard]] StringView             ExtensionOf(eImageFormat _format, bool _bIncludeDot = false);

}   // namespace jam