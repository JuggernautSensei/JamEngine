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
    m_vleLut.fill(k_invalidIndex);
    m_stride   = 0;
    m_numElems = 0;
}

void VertexLayout::Push(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    JAM_ASSERT(!Has(_attrib), "vertex attribute cannot be added twice.");

    VertexLayoutElement vlm = {};
    vlm.attrib              = _attrib;
    vlm.format              = _format;
    vlm.byteWidth           = ByteFrom(_format);
    vlm.offset              = m_stride;

    m_vleLut[IntFrom(_attrib)] = m_numElems;
    m_vles[m_numElems]         = vlm;

    m_stride += vlm.byteWidth;
    ++m_numElems;
}

void VertexLayout::Modify(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    JAM_ASSERT(Has(_attrib), "vertex attribute does not exist.");
    const Int32          idx          = m_vleLut[IntFrom(_attrib)];
    VertexLayoutElement& vleToMod     = m_vles[idx];
    const Int32          oldByteWidth = vleToMod.byteWidth;

    // 수정
    vleToMod.format           = _format;
    vleToMod.byteWidth        = ByteFrom(_format);
    const Int32 diffByteWidth = vleToMod.byteWidth - oldByteWidth;
    if (diffByteWidth == 0)
    {
        return; // 변경사항 없음
    }

    // stride 먼저 계산
    m_stride += diffByteWidth;

    // 이후의 모든 offset을 수정해야함.
    for (Int32 i = idx + 1; i < m_numElems; ++i)
    {
        VertexLayoutElement& vle = m_vles[i];
        vle.offset += diffByteWidth;
    }
}

void VertexLayout::Remove(
    const eVertexAttribute _attrib)
{
    JAM_ASSERT(Has(_attrib), "vertex attribute does not exist.");
    const Int32 idx = m_vleLut[IntFrom(_attrib)];

    const VertexLayoutElement& vleToRemove       = m_vles[idx];
    const Int32                byteWidthToRemove = vleToRemove.byteWidth;

    // stride 먼저 계산
    m_stride -= vleToRemove.byteWidth;

    // idx 이후의 모든 offset을 수정해야함.
    for (Int32 i = idx + 1; i < m_numElems; ++i)
    {
        VertexLayoutElement& vle = m_vles[i];
        vle.offset -= byteWidthToRemove;

        // 왼쪽으로 shift
        m_vles[i - 1]                 = vle;
        m_vleLut[IntFrom(vle.attrib)] = i - 1;   // 룩업 테이블도 갱신
    }

    // 마지막 요소 초기화
    m_vleLut[IntFrom(_attrib)] = k_invalidIndex;
    m_vles[m_numElems - 1]     = {};
    --m_numElems;
}

Int32 VertexLayout::GetStride() const
{
    return m_stride;
}

bool VertexLayout::Has(
    const eVertexAttribute _attrib) const
{
    return m_vleLut[IntFrom(_attrib)] != k_invalidIndex;
}

const VertexLayoutElement& VertexLayout::GetElement(
    const eVertexAttribute _attrib) const
{
    JAM_ASSERT(Has(_attrib), "vertex attribute does not exist.");
    const Int32 idx = m_vleLut[IntFrom(_attrib)];
    return m_vles[idx];
}

Int32 VertexLayout::GetNumElements() const
{
    return m_numElems;
}

Span<const VertexLayoutElement> VertexLayout::GetElements() const
{
    return { m_vles.data(), m_numElems };
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
