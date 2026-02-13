#pragma once
#include "lunasvg/lunasvg.h"

namespace Jam
{

struct SVGImporterDesc
{
    Int32 width           = -1;                    // desired width, -1 for original size
    Int32 height          = -1;                    // desired height, -1 for original size
    RGBA  backgroundColor = RGBA::k_transparent;   // background color
};

class SVGImporter
{
public:
    [[nodiscard]] static Expect<SVGImporter> ImportFromFile(const Path& _path, const SVGImporterDesc& _desc = {});
    [[nodiscard]] static Expect<SVGImporter> ImportFromSource(StringView _source, const SVGImporterDesc& _desc = {});

    [[nodiscard]] CMemoryBlock GetPixels() const;
    [[nodiscard]] Int32        GetWidth() const;
    [[nodiscard]] Int32        GetHeight() const;

private:
    SVGImporter() = default;
    Error Import_(std::unique_ptr<lunasvg::Document>&& _doc, const SVGImporterDesc& _desc);

    // result
    lunasvg::Bitmap m_bitmap = {};
};

}   // namespace Jam
