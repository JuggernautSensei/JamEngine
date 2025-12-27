#pragma once

namespace jam
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

    VertexLayout&      Push(eVertexAttribute _attrib, eAttributeFormat _format);
    [[nodiscard]] bool Has(eVertexAttribute _attrib) const;
    void               Reset();

    [[nodiscard]] Int32                                GetStride() const;
    [[nodiscard]] const VertexLayoutElement&           GetElement(eVertexAttribute _attrib) const;
    [[nodiscard]] std::span<const VertexLayoutElement> GetElements() const;
    [[nodiscard]] Int32                                GetNumElements() const;

private:
    static constexpr Int32 k_invalidElement = -1;

    Array<VertexLayoutElement, NumEnums<eVertexAttribute>()> m_vles     = {};
    Array<Int32, NumEnums<eVertexAttribute>()>               m_vleLut   = {};
    Int32                                                    m_numElems = 0;
    Int32                                                    m_stride   = 0;
};

[[nodiscard]] Int32 ByteFrom(eAttributeFormat _format);

}   // namespace jam