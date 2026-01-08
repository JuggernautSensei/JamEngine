#pragma once
#include "lunasvg/lunasvg.h"

namespace jam
{

class SVGImporter
{
public:
    bool Import(const fs::path& _path);
    bool ImportSource(StringView _source);

    void SetWidth(Int32 _width);
    void SetHeight(Int32 _height);
    void SetBackgroundColor(RGBA _rgba);

    [[nodiscard]] CMemView GetPixels() const;
    [[nodiscard]] Int32       GetWidth() const;
    [[nodiscard]] Int32       GetHeight() const;

private:
    // importer options
    Int32 m_width  = -1;
    Int32 m_height = -1;
    RGBA  m_bgCol  = RGBA::k_transparent;

    // result
    lunasvg::Bitmap m_bitmap = {};
};

}   // namespace jam
