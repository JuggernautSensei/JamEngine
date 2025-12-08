#include "pch.h"

#include "VertexLayout.h"

namespace jam
{

VertexLayout::VertexLayout()
{
    Reset();
}

void VertexLayout::Reset()
{
    std::memset(m_elems, 0, sizeof(m_elems));
    std::ranges::fill(m_elemLut, INVALID_ELEMENT);
    m_stride    = 0;
    m_elemCount = 0;
}

VertexLayout& VertexLayout::Add(const eVertexAttribute _attrib, const eAttributeFormat _format)
{
    JAM_ASSERT(_attrib != eVertexAttribute::Unknown, "vertex attribute cannot be unknown.");
    JAM_ASSERT(_format != eAttributeFormat::Unknown, "vertex attribute format cannot be unknown.");
    JAM_ASSERT(!Has(_attrib), "vertex attribute cannot be added twice.");

    VertexLayoutElement elem;
    elem.attrib    = _attrib;
    elem.format    = _format;
    elem.byteWidth = GetVertexAttributeFormatSize(_format);
    elem.offset    = m_stride;
    m_stride += elem.byteWidth;

    m_elemLut[EnumToInt(_attrib)] = m_elemCount;
    m_elems[m_elemCount]          = std::move(elem);
    ++m_elemCount;
    return *this;
}

Int32 VertexLayout::GetStride() const
{
    return m_stride;
}

bool VertexLayout::Has(const eVertexAttribute _attrib) const
{
    return m_elemLut[EnumToInt(_attrib)] != INVALID_ELEMENT;
}

const VertexLayoutElement& VertexLayout::GetElement(const eVertexAttribute _attrib) const
{
    JAM_ASSERT(Has(_attrib), "vertex attribute does not exist.");
    const Int32 index = m_elemLut[EnumToInt(_attrib)];
    return m_elems[index];
}

Int32 VertexLayout::GetElementCount() const
{
    return m_elemCount;
}

std::span<const VertexLayoutElement> VertexLayout::GetElements() const
{
    return { m_elems, static_cast<size_t>(m_elemCount) };
}

Int32 GetVertexAttributeFormatSize(const eAttributeFormat _format)
{
    switch (_format)
    {
        case eAttributeFormat::Float:
            return 4;
        case eAttributeFormat::Float2:
            return 8;
        case eAttributeFormat::Float3:
            return 12;
        case eAttributeFormat::Float4:
            return 16;
        case eAttributeFormat::UNorm8x4:
            return 4;
        case eAttributeFormat::UInt8x4:
            return 4;
        default:
            JAM_ASSERT(0, "unknown attribute format.");
            return 0;
    }
}

}   // namespace jam
