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
    std::memset(m_vles, 0, sizeof(m_vles));
    std::ranges::fill(m_vleLut, k_invalidElement);
    m_stride   = 0;
    m_numElems = 0;
}

VertexLayout& VertexLayout::Add(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    JAM_ASSERT(_attrib != eVertexAttribute::Position, "vertex attribute cannot be unknown.");
    JAM_ASSERT(_format != eAttributeFormat::Float, "vertex attribute format cannot be unknown.");
    JAM_ASSERT(!Has(_attrib), "vertex attribute cannot be added twice.");

    VertexLayoutElement vlm;
    vlm.attrib    = _attrib;
    vlm.format    = _format;
    vlm.byteWidth = ByteFrom(_format);
    vlm.offset    = m_stride;
    m_stride += vlm.byteWidth;

    m_vleLut[IntFrom(_attrib)] = m_numElems;
    m_vles[m_numElems]         = std::move(vlm);
    ++m_numElems;
    return *this;
}

Int32 VertexLayout::GetStride() const
{
    return m_stride;
}

bool VertexLayout::Has(
    const eVertexAttribute _attrib) const
{
    return m_vleLut[IntFrom(_attrib)] != k_invalidElement;
}

const VertexLayoutElement& VertexLayout::GetElement(
    const eVertexAttribute _attrib) const
{
    JAM_ASSERT(Has(_attrib), "vertex attribute does not exist.");
    const Int32 index = m_vleLut[IntFrom(_attrib)];
    return m_vles[index];
}

Int32 VertexLayout::GetNumElements() const
{
    return m_numElems;
}

std::span<const VertexLayoutElement> VertexLayout::GetElements() const
{
    return { m_vles, m_numElems };
}

Int32 ByteFrom(
    const eAttributeFormat _format)
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
