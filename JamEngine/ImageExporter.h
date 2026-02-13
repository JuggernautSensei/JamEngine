#pragma once
#include "File.h"
#include "ImageBase.h"

namespace Jam
{

class ImageExporter
{
public:
    static Error ExportToFile(const ImageContainer& _container, const Path& _path, eImageFormat _format, bool _bReplaceExtension, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None);
    static Error ExportToFile(TextureHandle _texh, const Path& _path, eImageFormat _format, bool _bReplaceExtension, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None);

    [[nodiscard]] static Expect<ImageBlob> ExportToMemory(const ImageContainer& _container, eImageFormat _format);
    [[nodiscard]] static Expect<ImageBlob> ExportToMemory(TextureHandle _texh, eImageFormat _format);

private:
    ImageExporter() = default;
};

}   // namespace Jam