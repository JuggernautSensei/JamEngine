#pragma once
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

    VertexLayout&      Add(eVertexAttribute _attrib, eAttributeFormat _format);
    [[nodiscard]] bool Has(eVertexAttribute _attrib) const;
    void               Reset();

    [[nodiscard]] Int32                           GetStride() const;
    [[nodiscard]] const VertexLayoutElement&      GetElem(eVertexAttribute _attrib) const;
    [[nodiscard]] Span<const VertexLayoutElement> GetElems() const;
    [[nodiscard]] Int32                           GetNumElems() const;
    [[nodiscard]] bool                            IsEmpty() const;
    [[nodiscard]] UInt64                          GetHash() const;

private:
    Array<VertexLayoutElement, CountOf<eVertexAttribute>()> m_vles   = {};   // vertex layout elements.
    EnumArray<eVertexAttribute, VertexLayoutElement*>       m_vleLut = {};   // vertex layout element lookup table.

    Int32 m_numElems = 0;
    Int32 m_stride   = 0;

    mutable bool   m_bDirty = true;
    mutable UInt64 m_hash   = 0;
};

[[nodiscard]] SizeT ByteWidthOf(eAttributeFormat _format);

}   // namespace Jam