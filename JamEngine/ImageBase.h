#pragma once
#include "GraphicsBase.h"

namespace Jam
{

using ImageContainer = DirectX::ScratchImage;
using ImageBlob      = DirectX::Blob;

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

[[nodiscard]] Optional<eImageFormat> ParseAsImageFormat(const Path& _path);
[[nodiscard]] Optional<eImageFormat> ParseAsImageFormat(StringView _ext);
[[nodiscard]] StringView             ExtensionOf(eImageFormat _format);   // include dot

}   // namespace Jam
