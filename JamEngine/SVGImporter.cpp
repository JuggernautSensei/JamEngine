#include "pch.h"
#include "SVGImporter.h"

#include "Graphics.h"
#include "Profile.h"

#include "lunasvg/lunasvg.h"

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

    lunasvg::Bitmap bmp = doc->renderToBitmap(m_width, m_height, m_bgCol.ToRGBA());
    if (bmp.isNull())
    {
        return false;
    }

    JAM_ASSERT(bmp.stride() % 4 == 0, "Unexpected stride in rendered SVG bitmap");
    LoadTexture_(bmp.width(), bmp.height(), { bmp.data(), bmp.height() * bmp.stride() });
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

    lunasvg::Bitmap bmp = doc->renderToBitmap(m_width, m_height, m_bgCol.ToRGBA());
    if (bmp.isNull())
    {
        return false;
    }

    JAM_ASSERT(bmp.stride() % 4 == 0, "Unexpected stride in rendered SVG bitmap");
    LoadTexture_(bmp.width(), bmp.height(), { bmp.data(), bmp.height() * bmp.stride() });
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

void SVGImporter::LoadTexture_(
    const Int32    _width,
    const Int32    _height,
    const CMemView _mem)
{
    CMemView mems[] = { _mem };
    m_texh          = Graphics::CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, 1, 0, mems);
}

}   // namespace jam