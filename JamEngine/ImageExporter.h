#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"

namespace jam
{

// todo : maybe support more export options later

class ImageExporter
{
public:
    bool Export(TextureHandle _texh, const fs::path& _path, eImageFormat _format, bool _bChangeExtenstion = true) const;

private:
    [[nodiscard]] DirectX::WICCodecs WICCodecsFrom_(eImageFormat _format) const;

    TextureHandle m_texh = k_invalidHandle;
};

}   // namespace jam