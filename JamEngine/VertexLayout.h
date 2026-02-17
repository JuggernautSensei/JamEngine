#pragma once
#include "Archive.h"
#include "EnumArray.h"

namespace Jam
{

enum class eVertexAttribute
{
    Position = 0,
    Normal,
    Tangent,
    Bitangent,
    Color0,
    Color1,
    Color2,
    Color3,
    BlendIndex,
    BlendWeight,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
};

enum class eAttributeFormat
{
    Float = 0,
    Float2,
    Float3,
    Float4,
    UNorm8x4,
    UInt8x4,
};

struct VertexLayoutElement
{
    eVertexAttribute attrib    = eVertexAttribute::Position;
    eAttributeFormat format    = eAttributeFormat::Float;
    Int32            byteWidth = 0;
    Int32            offset    = 0;
};

class VertexLayout
{
public:
    VertexLayout();

    void               Push(eVertexAttribute _attrib, eAttributeFormat _format);
    void               Modify(eVertexAttribute _attrib, eAttributeFormat _format);
    void               Remove(eVertexAttribute _attrib);
    [[nodiscard]] bool Has(eVertexAttribute _attrib) const;
    void               Reset();

    [[nodiscard]] Int32                           GetStride() const;
    [[nodiscard]] const VertexLayoutElement&      GetElement(eVertexAttribute _attrib) const;
    [[nodiscard]] Span<const VertexLayoutElement> GetElements() const;
    [[nodiscard]] Int32                           GetNumElements() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    static constexpr Int32 k_invalidIdx = -1;

    Array<VertexLayoutElement, CountOf<eVertexAttribute>()> m_vles   = {};   // 레이아웃의 요소는 offset 순서대로 정렬됨
    EnumArray<eVertexAttribute, Int32>                      m_vleLut = {};   // O(1)로 요소에 접근하기 위한 룩업 테이블. attribute는 VertexLayout 에서 유일함. 그러므로 역매핑 가능

    Int32 m_numElems = 0;
    Int32 m_stride   = 0;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(VertexLayout);

[[nodiscard]] SizeT ByteWidthOf(eAttributeFormat _format);

}   // namespace Jam