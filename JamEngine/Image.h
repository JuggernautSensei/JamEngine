#pragma once
#include "File.h"

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

[[nodiscard]] Optional<eImageFormat> ParseAsImageFormat(StringView _ext);
[[nodiscard]] StringView             ExtensionOf(eImageFormat _format);   // include dot

class Image
{
public:
    explicit Image(ImageContainer&& _texh);

    [[nodiscard]] static Expect<Image> ImportFromFile(StringView _path, eImageFormat _format);
    [[nodiscard]] static Expect<Image> ImportFromFile(StringView _path);
    [[nodiscard]] static Expect<Image> ImportFromMemory(MemoryView _mem, eImageFormat _format);

    Error             ExportToFile(StringView _path, eImageFormat _format, eFileWrite _mode = eFileWrite::Truncate) const;
    Expect<ImageBlob> ExportToMemory(eImageFormat _format) const;

    [[nodiscard]] const ImageContainer& GetImageContainer() const;

private:
    Image() = default;
    Error ImportFromFile_(StringView _path, eImageFormat _format);
    Error ImportFromFile_(StringView _path);
    Error ImportFromMemory_(MemoryView _mem, eImageFormat _format);

    ImageContainer m_container = {};
};

}   // namespace Jam
