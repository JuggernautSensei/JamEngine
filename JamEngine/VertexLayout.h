#pragma once

namespace jam
{

enum class eVertexAttribute
{
    Unknown  = -1,
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
    Unknown = -1,
    Float   = 0,
    Float2,
    Float3,
    Float4,
    UNorm8x4,
    UInt8x4,
};

struct VertexLayoutElement
{
    eVertexAttribute attrib    = eVertexAttribute::Unknown;
    eAttributeFormat format    = eAttributeFormat::Unknown;
    Int32            byteWidth = 0;
    Int32            offset    = 0;
};

class VertexLayout
{
    enum Magic
    {
        INVALID_ELEMENT = -1
    };

public:
    VertexLayout();

    VertexLayout&      Add(eVertexAttribute _attrib, eAttributeFormat _format);
    [[nodiscard]] bool Has(eVertexAttribute _attrib) const;
    void               Reset();

    [[nodiscard]] Int32                                GetStride() const;
    [[nodiscard]] Int32                                GetElementCount() const;
    [[nodiscard]] const VertexLayoutElement&           GetElement(eVertexAttribute _attrib) const;
    [[nodiscard]] std::span<const VertexLayoutElement> GetElements() const;

private:
    VertexLayoutElement m_elems[EnumCount<eVertexAttribute>()]   = {};
    Int32               m_elemLut[EnumCount<eVertexAttribute>()] = {};   // lookup table for attribute to m_elems index
    Int32               m_elemCount                              = 0;
    Int32               m_stride                                 = 0;
};

[[nodiscard]] Int32 GetVertexAttributeFormatSize(eAttributeFormat _format);

}   // namespace jam