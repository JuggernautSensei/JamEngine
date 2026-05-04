#pragma once
#include <random>

namespace Jam
{

struct RandomDeviceSeed
{
    struct Tag
    {
    };

    explicit constexpr RandomDeviceSeed(Tag) {}
};

inline constexpr RandomDeviceSeed kRandomDeviceSeed { RandomDeviceSeed::Tag {} };

namespace Detail
{

    [[nodiscard]] UInt64 MakeU64FromRd() noexcept;

}   // namespace Detail

// George Marsaglia's SHR3 (fastest)
class RngSHR3
{
public:
    explicit RngSHR3(const UInt64 _seed = 342211234567ULL) noexcept
        : m_jsr(_seed)
    {
    }

    explicit RngSHR3(RandomDeviceSeed) noexcept
        : m_jsr(Detail::MakeU64FromRd())
    {
    }

    void Reseed(const UInt64 _seed = 342211234567ULL) noexcept
    {
        m_jsr = _seed;
    }

    void Reseed(RandomDeviceSeed) noexcept
    {
        m_jsr = Detail::MakeU64FromRd();
    }

    [[nodiscard]] UInt64 Generate() noexcept
    {
        m_jsr ^= (m_jsr << 13);
        m_jsr ^= (m_jsr >> 7);
        m_jsr ^= (m_jsr << 17);
        return m_jsr;
    }

private:
    UInt64 m_jsr = 0;
};

// George Marsaglia's MWC (more random)
class RngMWC
{
public:
    explicit RngMWC(
        const UInt64 _seedZ = 123456789123ULL,
        const UInt64 _seedW = 654356789123ULL) noexcept
        : m_z(_seedZ)
        , m_w(_seedW)
    {
    }

    explicit RngMWC(RandomDeviceSeed) noexcept
        : m_z(Detail::MakeU64FromRd())
        , m_w(Detail::MakeU64FromRd())
    {
    }

    void Reseed(
        const UInt64 _seedZ = 123456789123ULL,
        const UInt64 _seedW = 654356789123ULL) noexcept
    {
        m_z = _seedZ;
        m_w = _seedW;
    }

    void Reseed(RandomDeviceSeed) noexcept
    {
        m_z = Detail::MakeU64FromRd();
        m_w = Detail::MakeU64FromRd();
    }

    [[nodiscard]] UInt64 Generate() noexcept
    {
        m_z = 698769069ULL * (m_z & 0xFFFFFFFFULL) + (m_z >> 32);
        m_w = 362436069ULL * (m_w & 0xFFFFFFFFULL) + (m_w >> 32);
        return (m_z << 32) + m_w;
    }

private:
    UInt64 m_z = 0;
    UInt64 m_w = 0;
};

// Mersenne Twister (highest quality)
class RngMt19937
{
public:
    explicit RngMt19937(const UInt64 _seed = 123456789ULL) noexcept
        : m_mt(_seed)
    {
    }

    explicit RngMt19937(RandomDeviceSeed) noexcept
        : m_mt(Detail::MakeU64FromRd())
    {
    }

    void Reseed(const UInt64 _seed = 123456789ULL) noexcept
    {
        m_mt.seed(_seed);
    }

    void Reseed(RandomDeviceSeed) noexcept
    {
        m_mt = std::mt19937_64(Detail::MakeU64FromRd());
    }

    [[nodiscard]] UInt64 Generate() noexcept
    {
        return m_mt();
    }

private:
    std::mt19937_64 m_mt;
};

template<typename T>
concept IsRng = requires(T t) {
    { t.Generate() } -> std::same_as<UInt64>;
};

template<typename T, typename TRng>
    requires IsRng<TRng> && std::is_arithmetic_v<T>
[[nodiscard]] T GenerateRandom(TRng& _rng, const T _min, const T _max) noexcept
{
    JAM_ASSERT(_min <= _max, "Invalid range! min: {}, max: {}", _min, _max);

    if (_min == _max)
    {
        return _min;
    }

    if constexpr (std::is_integral_v<T>)
    {
        const UInt64 range = static_cast<UInt64>(_max - _min) + 1;
        return static_cast<T>(_min + static_cast<T>(_rng.Generate() % range));
    }
    else
    {
        using FloatT     = std::conditional_t<std::is_same_v<T, float>, float, double>;
        const FloatT f01 = static_cast<FloatT>(_rng.Generate()) / static_cast<FloatT>(Limits<UInt64>::kHighest);
        return static_cast<T>(_min + f01 * (_max - _min));
    }
}

}   // namespace Jam