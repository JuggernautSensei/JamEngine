#pragma once
#include "GraphicsBase.h"
#include "lunasvg/lunasvg.h"

namespace Jam
{

struct SVGImportInfo
{
    Int32 widthOrZero     = 0;                    // desired width, 0 for original size
    Int32 heightOrZero    = 0;                    // desired height, 0 for original size
    RGBA  backgroundColor = RGBA::kTransparent;   // background color
};

class SVGImage
{
public:
    [[nodiscard]] static Expect<SVGImage> ImportFromFile(StringView _path, const SVGImportInfo& _info = {});
    [[nodiscard]] static Expect<SVGImage> ImportFromSource(StringView _source, const SVGImportInfo& _info = {});

    [[nodiscard]] MemoryView GetPixels() const;
    [[nodiscard]] Int32      GetWidth() const;
    [[nodiscard]] Int32      GetHeight() const;

private:
    SVGImage() = default;
    Error Import_(std::unique_ptr<lunasvg::Document>&& _doc, const SVGImportInfo& _desc);

    // result
    lunasvg::Bitmap m_bitmap = {};
};

}   // namespace Jam
