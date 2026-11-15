#pragma once

namespace Jam
{

namespace Details
{
    constexpr UInt64 kFnv1ah64Prime = 1099511628211ULL;
    constexpr UInt64 kFnv1ah64Seed  = 14695981039346656037ULL;
}   // namespace Details

template<typename TChar>
class Fnv1ahHasher
{
public:
    explicit constexpr Fnv1ahHasher(
        const UInt64 _seed = Details::kFnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(_str[i])) * Details::kFnv1ah64Prime;
        }
    }

    constexpr void Mix(
        const TChar* _str)
    {
        for (; *_str; ++_str)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(*_str)) * Details::kFnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

template<typename TChar>
class IgnoreCaseFnv1ahHasher
{
    using CharTraits = CharTraits<TChar>;

public:
    explicit constexpr IgnoreCaseFnv1ahHasher(
        const UInt64 _seed = Details::kFnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(CharTraits::ToLower(_str[i]))) * Details::kFnv1ah64Prime;
        }
    }

    constexpr void Mix(
        const TChar* _str)
    {
        for (; *_str; ++_str)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(CharTraits::ToLower(*_str))) * Details::kFnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

}   // namespace Jam