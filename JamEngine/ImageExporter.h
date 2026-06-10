#pragma once
#include "File.h"
#include "GraphicsBase.h"
#include "ImageBase.h"

namespace Jam
{

class ImageExporter
{
public:
    static Error ExportToFile(const ImageContainer& _container, StringView _path, eImageFormat _format, bool _bReplaceExtension = false, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);
    static Error ExportToFile(TextureHandle _texh, StringView _path, eImageFormat _format, bool _bReplaceExtension = false, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);

    [[nodiscard]] static Expect<ImageBlob> ExportToMemory(const ImageContainer& _container, eImageFormat _format);
    [[nodiscard]] static Expect<ImageBlob> ExportToMemory(TextureHandle _texh, eImageFormat _format);

private:
    ImageExporter() = default;
};

}   // namespace Jam