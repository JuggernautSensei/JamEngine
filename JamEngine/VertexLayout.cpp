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

    VertexLayoutElement vlm = {};
    vlm.attrib              = _attrib;
    vlm.format              = _format;
    vlm.byteWidth           = static_cast<Int32>(ByteWidthOf(_format));
    vlm.offset              = m_stride;
    m_vles[m_numElems++]    = vlm;
    m_stride += vlm.byteWidth;
    m_vleLut[_attrib] = &m_vles[m_numElems - 1];
    m_bDirty          = true;
    return *this;
}

bool VertexLayout::Has(
    const eVertexAttribute _attrib) const
{
    return m_vleLut[_attrib] != nullptr;
}

void VertexLayout::Reset()
{
    m_vles.fill({});
    m_vleLut.Fill(nullptr);
    m_stride   = 0;
    m_numElems = 0;
    m_hash     = 0;
    m_bDirty   = true;
}

Int32 VertexLayout::GetStride() const
{
    return m_stride;
}

const VertexLayoutElement& VertexLayout::GetElem(
    const eVertexAttribute _attrib) const
{
    JAM_ASSERT(Has(_attrib), "vertex attribute '{}' does not exist in this vertex layout.", NameOf(_attrib));
    return *m_vleLut[_attrib];
}

Span<const VertexLayoutElement> VertexLayout::GetElems() const
{
    return { m_vles.data(), m_numElems };
}

Int32 VertexLayout::GetNumElems() const
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
        for (const VertexLayoutElement& elem: GetElems())
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
