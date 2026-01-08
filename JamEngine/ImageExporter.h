#pragma once
#include "FileIOUtilities.h"
#include "GraphicsBase.h"
#include "ImageBase.h"

namespace jam
{

// todo : maybe support more export options later

class ImageExporter
{
public:
    bool Export(TextureHandle _texh, const fs::path& _path, eImageFormat _format, bool _bReplaceExtension = true, eSaveFileFlags _flags = eSaveFileFlags_None) const;
    bool Export(const DirectX::ScratchImage& _image, const fs::path& _path, eImageFormat _format, bool _bReplaceExtension = true, eSaveFileFlags _flags = eSaveFileFlags_None) const;

private:
    bool                             Export_(const DirectX::ScratchImage& _scratchImg, const fs::path& _path, eImageFormat _format) const;
    [[nodiscard]] DirectX::WICCodecs WICCodecsFrom_(eImageFormat _format) const;
};

}   // namespace jam