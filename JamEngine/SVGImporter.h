#pragma once
#include "IImporter.h"

namespace jam
{

class SVGImporter : public IImporter
{
public:
    bool Import(const fs::path& _path) override;
    bool ImportSource(StringView _source);

    void SetWidth(Int32 _width);
    void SetHeight(Int32 _height);
    void SetBackgroundColor(RGBA _rgba);

private:
    void LoadTexture_(Int32 _width, Int32 _height, CMemView _mem);

    Int32         m_width  = -1;
    Int32         m_height = -1;
    RGBA          m_bgCol  = RGBA::k_transparent;
    TextureHandle m_texh   = k_invalidHandle;
};

}   // namespace jam
