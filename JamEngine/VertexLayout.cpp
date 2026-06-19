#include "pch.h"

#include "VertexLayout.h"

namespace Jam
{

VertexLayout::VertexLayout()
{
    Reset();
}

VertexLayout& VertexLayout::Add(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    JAM_ASSERT(!Has(_attrib), "vertex attribute cannot be added twice.");

    VertexElement ve    = {};
    ve.attrib           = _attrib;
    ve.format           = _format;
    ve.byteWidth        = static_cast<Int32>(ByteWidthOf(_format));
    ve.offset           = m_stride;
    m_ves[m_numElems++] = ve;
    m_stride += ve.byteWidth;
    m_veLUT[_attrib] = &m_ves[m_numElems - 1];
    m_bDirty         = true;
    return *this;
}

bool VertexLayout::Has(
    const eVertexAttribute _attrib) const
{
    return m_veLUT[_attrib] != nullptr;
}

void VertexLayout::Reset()
{
    m_ves.fill({});
    m_veLUT.Fill(nullptr);
    m_stride   = 0;
    m_numElems = 0;
    m_hash     = 0;
    m_bDirty   = true;
}

Int32 VertexLayout::GetStride() const
{
    return m_stride;
}

const VertexElement& VertexLayout::GetElement(
    const eVertexAttribute _attrib) const
{
    JAM_ASSERT(Has(_attrib), "vertex attribute '{}' does not exist in this vertex layout.", NameOf(_attrib));
    return *m_veLUT[_attrib];
}

Span<const VertexElement> VertexLayout::GetElements() const
{
    return { m_ves.data(), m_numElems };
}

Int32 VertexLayout::GetNumElements() const
{
    return m_numElems;
}

bool VertexLayout::IsEmpty() const
{
    return m_numElems == 0;
}

UInt64 VertexLayout::GetHash() const
{
    if (m_bDirty)
    {
        Murmur3Hasher hasher {};
        for (const VertexElement& elem: GetElements())
        {
            hasher.Mix(AsMemoryView(elem.attrib));
            hasher.Mix(AsMemoryView(elem.format));
        }
        m_hash   = hasher.Finish();
        m_bDirty = false;
    }
    return m_hash;
}

SizeT ByteWidthOf(
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
            JAM_UNREACHABLE;
    }
}

}   // namespace Jam
