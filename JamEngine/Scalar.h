#pragma once

namespace Jam
{

enum class eScalarType
{
    Null = 0,
    Bool,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
};

struct NullT
{
};

constexpr NullT k_null = {};

template<typename T>
constexpr auto IsScalarType = std::is_arithmetic_v<T> || std::is_same_v<T, NullT>;

template<typename T>
    requires IsScalarType<T>
struct ScalarTraits
{
};

#define JAM_DEFINE_SCALAR_TRAITS(_type, _enumType)     \
    template<>                                         \
    struct ScalarTraits<_type>                         \
    {                                                  \
        static constexpr eScalarType type = _enumType; \
    }

JAM_DEFINE_SCALAR_TRAITS(NullT, eScalarType::Null);
JAM_DEFINE_SCALAR_TRAITS(bool, eScalarType::Bool);
JAM_DEFINE_SCALAR_TRAITS(Int8, eScalarType::Int8);
JAM_DEFINE_SCALAR_TRAITS(UInt8, eScalarType::UInt8);
JAM_DEFINE_SCALAR_TRAITS(Int16, eScalarType::Int16);
JAM_DEFINE_SCALAR_TRAITS(UInt16, eScalarType::UInt16);
JAM_DEFINE_SCALAR_TRAITS(Int32, eScalarType::Int32);
JAM_DEFINE_SCALAR_TRAITS(UInt32, eScalarType::UInt32);
JAM_DEFINE_SCALAR_TRAITS(Int64, eScalarType::Int64);
JAM_DEFINE_SCALAR_TRAITS(UInt64, eScalarType::UInt64);
JAM_DEFINE_SCALAR_TRAITS(float, eScalarType::Float);

class Scalar
{
public:
    /* implicit */ Scalar(const NullT)
        : m_type(eScalarType::Null)
        , m_value { .m_null = k_null }
    {
    }

    /* implicit */ Scalar(const bool _v)
        : m_type(eScalarType::Bool)
        , m_value { .m_b = _v }
    {
    }

    /* implicit */ Scalar(const Int8 _v)
        : m_type(eScalarType::Int8)
        , m_value { .m_s8 = _v }
    {
    }

    /* implicit */ Scalar(const UInt8 _v)
        : m_type(eScalarType::UInt8)
        , m_value { .m_u8 = _v }
    {
    }

    /* implicit */ Scalar(const Int16 _v)
        : m_type(eScalarType::Int16)
        , m_value { .m_s16 = _v }
    {
    }

    /* implicit */ Scalar(const UInt16 _v)
        : m_type(eScalarType::UInt16)
        , m_value { .m_u16 = _v }
    {
    }

    /* implicit */ Scalar(const Int32 _v)
        : m_type(eScalarType::Int32)
        , m_value { .m_s32 = _v }
    {
    }

    /* implicit */ Scalar(const UInt32 _v)
        : m_type(eScalarType::UInt32)
        , m_value { .m_u32 = _v }
    {
    }

    /* implicit */ Scalar(const Int64 _v)
        : m_type(eScalarType::Int64)
        , m_value { .m_s64 = _v }
    {
    }

    /* implicit */ Scalar(const UInt64 _v)
        : m_type(eScalarType::UInt64)
        , m_value { .m_u64 = _v }
    {
    }

    /* implicit */ Scalar(const float _v)
        : m_type(eScalarType::Float)
        , m_value { .m_f = _v }
    {
    }

    [[nodiscard]] bool operator==(const Scalar _other) const
    {
        return CmpEquel(_other);
    }

    [[nodiscard]] bool operator!=(const Scalar _other) const
    {
        return !CmpEquel(_other);
    }

    [[nodiscard]] bool CmpEquel(const Scalar& _other) const
    {
        return m_type == _other.m_type && MemEqual(m_value, _other.m_value);
    }

    [[nodiscard]] bool CmpEquelApprox(const Scalar& _other, const float _tolerance = k_epsilon) const
    {
        if (m_type != _other.m_type)
        {
            return false;
        }

        if (IsFloatingPoint())
        {
            return IsEqualApprox(m_value.m_f, _other.m_value.m_f, _tolerance);
        }

        return MemEqual(m_value, _other.m_value);
    }

    template<typename T>
        requires IsScalarType<T>
    [[nodiscard]] T GetValue() const
    {
        JAM_ASSERT(m_type == ScalarTraits<T>::type, "Scalar type mismatch");
        return *static_cast<const T*>(GetRawPtr());
    }

    [[nodiscard]] eScalarType GetType() const
    {
        return m_type;
    }

    [[nodiscard]] void* GetRawPtr()
    {
        return &m_value;
    }

    [[nodiscard]] const void* GetRawPtr() const
    {
        return &m_value;
    }

    [[nodiscard]] bool IsFloatingPoint() const
    {
        return m_type == eScalarType::Float;
    }

    [[nodiscard]] bool IsInteger() const
    {
        return m_type == eScalarType::Int8 || m_type == eScalarType::UInt8
            || m_type == eScalarType::Int16 || m_type == eScalarType::UInt16
            || m_type == eScalarType::Int32 || m_type == eScalarType::UInt32
            || m_type == eScalarType::Int64 || m_type == eScalarType::UInt64;
    }

    [[nodiscard]] bool IsBoolean() const
    {
        return m_type == eScalarType::Bool;
    }

    [[nodiscard]] bool IsNull() const
    {
        return m_type == eScalarType::Null;
    }

private:
    eScalarType m_type = eScalarType::Null;
    union
    {
        NullT  m_null = k_null;
        bool   m_b;
        Int8   m_s8;
        UInt8  m_u8;
        Int16  m_s16;
        UInt16 m_u16;
        Int32  m_s32;
        UInt32 m_u32;
        Int64  m_s64;
        UInt64 m_u64;
        float  m_f;
    } m_value;
};

[[nodiscard]] Scalar HighestValueOf(eScalarType _type);
[[nodiscard]] Scalar LowestValueOf(eScalarType _type);
[[nodiscard]] Scalar MinValueOf(eScalarType _type);
[[nodiscard]] Scalar MaxValueOf(eScalarType _type);
[[nodiscard]] Int32  ByteOf(eScalarType _type);

}   // namespace Jam