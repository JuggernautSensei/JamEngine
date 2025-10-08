#pragma once

namespace jam
{

/// @brief 버텍스 속성 열거형
enum class eVertexAttribute : Int8
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

/// @brief 버텍스 속성 데이터 타입 열거형
enum class eAttributeFormat : Int8
{
    Unknown = -1,
    Float   = 0,
    Float2,
    Float3,
    Float4,
    UNorm8x4,
    UInt8x4,
};

/// @brief eAttributeFormat 별 데이터 크기 테이블
constexpr Int32 k_formatSizeTable[EnumCount<eAttributeFormat>()] = {
    sizeof(float),       // Float
    sizeof(float) * 2,   // Float2
    sizeof(float) * 3,   // Float3
    sizeof(float) * 4,   // Float4
    sizeof(UInt8) * 4,   // UNorm8x4
    sizeof(UInt8) * 4,   // UInt8x4
};

/// @brief 버텍스 속성
struct VertexLayoutElement
{
    eVertexAttribute attrib    = eVertexAttribute::Unknown;
    eAttributeFormat format    = eAttributeFormat::Unknown;
    Int32            byteWidth = 0;
    Int32            offset    = 0;
};

/// @brief 버텍스 레이아웃 클래스
/// 버텍스의 속성을 정의합니다.
class VertexLayout
{
public:
    /// @brief 기본 생성자
    VertexLayout() = default;

    /// @brief 초기화
    void Reset()
    {
        std::memset(m_elems, 0, sizeof(m_elems));
        std::memset(m_attribRTable, k_int32Max, sizeof(m_attribRTable));
        m_stride    = 0;
        m_elemCount = 0;
    }

    /// @brief 속성추가
    VertexLayout& Add(const eVertexAttribute _attrib, const eAttributeFormat _format)
    {
        JAM_ASSERT(_attrib != eVertexAttribute::Unknown, "버텍스 속성은 Unknown이 될 수 없습니다.");
        JAM_ASSERT(_format != eAttributeFormat::Unknown, "버텍스 속성 포맷은 Unknown이 될 수 없습니다.");
        JAM_ASSERT(!Has(_attrib), "같은 버텍스 속성을 중복해서 추가할 수 없습니다.");

        VertexLayoutElement elem;
        elem.attrib    = _attrib;
        elem.format    = _format;
        elem.byteWidth = k_formatSizeTable[EnumToInt(_format)];
        elem.offset    = m_stride;
        m_stride += elem.byteWidth;

        m_attribRTable[EnumToInt(_attrib)] = m_elemCount;
        m_elems[m_elemCount]               = std::move(elem);
        ++m_elemCount;
        return *this;
    }

    /// @brief 버텍스 스트라이드(바이트 단위) 가져오기
    JAM_NODISCARD Int32 GetStride() const
    {
        return m_stride;
    }

    JAM_NODISCARD bool Has(const eVertexAttribute _attrib) const
    {
        return m_attribRTable[EnumToInt(_attrib)] != k_int32Max;
    }

    JAM_NODISCARD const VertexLayoutElement& GetElement(const eVertexAttribute _attrib) const
    {
        JAM_ASSERT(Has(_attrib), "해당 버텍스 속성이 존재하지 않습니다.");
        const Int32 index = m_attribRTable[EnumToInt(_attrib)];
        return m_elems[index];
    }

    JAM_NODISCARD Int32 GetElementCount() const
    {
        return m_elemCount;
    }

    JAM_NODISCARD std::span<const VertexLayoutElement> GetElements() const
    {
        return { m_elems, static_cast<size_t>(m_elemCount) };
    }

private:
    VertexLayoutElement m_elems[EnumCount<eVertexAttribute>()]        = {};
    Int32               m_attribRTable[EnumCount<eVertexAttribute>()] = { k_int32Max };
    Int32               m_elemCount                                   = 0;
    Int32               m_stride                                      = 0;
};

}   // namespace jam