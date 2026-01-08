#include "pch.h"
#include "SVGImporter.h"

#include "Graphics.h"
#include "Profile.h"

namespace jam
{

bool SVGImporter::Import(
    const fs::path& _path)
{
    JAM_FUNCTION_PROFILE();

    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromFile(_path.native());
    if (!doc)
    {
        return false;
    }

    m_bitmap = doc->renderToBitmap(m_width, m_height, m_bgCol.ToRGBA());
    if (m_bitmap.isNull())
    {
        return false;
    }

    JAM_ASSERT(m_bitmap.stride() % 4 == 0, "Unexpected stride in rendered SVG bitmap");
    return true;
}

bool SVGImporter::ImportSource(
    const StringView _source)
{
    JAM_FUNCTION_PROFILE();

    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromData(_source.GetPtr(), _source.GetLength());
    if (!doc)
    {
        return false;
    }

    m_bitmap = doc->renderToBitmap(m_width, m_height, m_bgCol.ToRGBA());
    if (m_bitmap.isNull())
    {
        return false;
    }

    JAM_ASSERT(m_bitmap.stride() % 4 == 0, "Unexpected stride in rendered SVG bitmap");
    return true;
}

void SVGImporter::SetWidth(
    const Int32 _width)
{
    m_width = _width;
}

void SVGImporter::SetHeight(
    const Int32 _height)
{
    m_height = _height;
}

void SVGImporter::SetBackgroundColor(
    const RGBA _rgba)
{
    m_bgCol = _rgba;
}

CMemView SVGImporter::GetPixels() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return { m_bitmap.data(), m_bitmap.stride() * m_bitmap.height() };
}

Int32 SVGImporter::GetWidth() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return m_bitmap.width();
}

Int32 SVGImporter::GetHeight() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return m_bitmap.height();
}

}   // namespace jam