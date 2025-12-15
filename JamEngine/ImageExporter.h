#pragma once
#include "GraphicsBase.h"
#include "IExporter.h"

namespace jam
{

class ImageExporter : public IExporter
{
public:
    bool Export() override;
    void SetPath(const fs::path& _path) override;
    void SetTexture(TextureHandle _texh);

private:
    [[nodiscard]] DirectX::WICCodecs WICCodecsFrom_(eImageFormat _format) const;

    TextureHandle m_texh = k_invalidHandle;
    fs::path      m_path = {};
};

}   // namespace jam