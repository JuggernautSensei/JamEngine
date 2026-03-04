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
    using ElemArray  = Array<VertexLayoutElement, CountOf<eVertexAttribute>()>;
    using ElemLookup = EnumArray<eVertexAttribute, VertexLayoutElement*>;

    VertexLayout();
    void               Push(eVertexAttribute _attrib, eAttributeFormat _format);
    [[nodiscard]] bool Has(eVertexAttribute _attrib) const;
    void               Reset();

    [[nodiscard]] Int32                           GetStride() const;
    [[nodiscard]] const VertexLayoutElement&      GetElem(eVertexAttribute _attrib) const;
    [[nodiscard]] Span<const VertexLayoutElement> GetElems() const;
    [[nodiscard]] Int32                           GetNumElems() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    ElemArray  m_vles   = {};   // vertex layout elements.
    ElemLookup m_vleLut = {};   // vertex layout element lookup table.

    Int32 m_numElems = 0;
    Int32 m_stride   = 0;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(VertexLayout);

[[nodiscard]] SizeT ByteWidthOf(eAttributeFormat _format);

}   // namespace Jam