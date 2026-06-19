#include "pch.h"
#include "SVGImage.h"

namespace Jam
{

Expect<SVGImage> SVGImage::ImportFromFile(
    StringView           _path,
    const SVGImportInfo& _info)
{
    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromFile(_path.GetPtr());
    if (!doc)
    {
        JAM_LOG_ERROR("SVGImporter::ImportFromFile: failed to load SVG file: {}", _path);
        return Error::ImportFailed;
    }

    SVGImage importer = {};
    if (const Error err = importer.Import_(std::move(doc), _info))
    {
        return err;
    }
    return importer;
}

Expect<SVGImage> SVGImage::ImportFromSource(
    const StringView     _source,
    const SVGImportInfo& _info)
{
    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromData(_source.GetPtr(), _source.GetLength());
    if (!doc)
    {
        JAM_LOG_ERROR("SVGImporter::ImportFromSource: failed to load SVG source data.");
        return Error::ImportFailed;
    }

    SVGImage importer = {};
    if (const Error err = importer.Import_(std::move(doc), _info))
    {
        return err;
    }

    return importer;
}

MemoryView SVGImage::GetPixels() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return { m_bitmap.data(), m_bitmap.stride() * m_bitmap.height() };
}

Int32 SVGImage::GetWidth() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return m_bitmap.width();
}

Int32 SVGImage::GetHeight() const
{
    JAM_ASSERT(!m_bitmap.isNull(), "SVG image not imported yet");
    return m_bitmap.height();
}

Error SVGImage::Import_(
    std::unique_ptr<lunasvg::Document>&& _doc,
    const SVGImportInfo&                 _desc)
{
    JAM_ASSERT(_doc, "Invalid SVG document");

    const Int32  w  = _desc.widthOrZero > 0 ? _desc.widthOrZero : -1;
    const Int32  h  = _desc.heightOrZero > 0 ? _desc.heightOrZero : -1;
    const UInt32 bg = _desc.backgroundColor.ToPackedRGBA();
    m_bitmap        = _doc->renderToBitmap(w, h, bg);
    if (m_bitmap.isNull())
    {
        JAM_LOG_ERROR("SVGImporter::Import_: failed to render SVG document to bitmap.");
        return Error::ImportFailed;
    }

    JAM_ASSERT(m_bitmap.stride() % 4 == 0, "Unexpected stride in rendered SVG bitmap");
    return Error::None;
}

}   // namespace Jam