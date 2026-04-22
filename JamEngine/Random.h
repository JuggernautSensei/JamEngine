#pragma once

namespace Jam
{

// George Marsaglia's MWC (more random)
class RngMWC
{
public:
    explicit RngMWC(
        const UInt32 _seedZ = 12345,
        const UInt32 _seedW = 65435)
        : m_z(_seedZ)
        , m_w(_seedW)
    {
    }

    void Reseed(
        const UInt32 _seedZ = 12345,
        const UInt32 _seedW = 65435)
    {
        m_z = _seedZ;
        m_w = _seedW;
    }

    [[nodiscard]] UInt32 Generate()
    {
        m_z = 36969 * (m_z & 65535) + (m_z >> 16);
        m_w = 18000 * (m_w & 65535) + (m_w >> 16);
        return (m_z << 16) + m_w;
    }

private:
    UInt32 m_z = 0;
    UInt32 m_w = 0;
};

// George Marsaglia's SHR3 (more fast)
class RngSHR3
{
public:
    explicit RngSHR3(
        const UInt32 _seed = 34221)
        : m_jsr(_seed)
    {
    }

    void Reseed(
        const UInt32 _seed = 34221)
    {
        m_jsr = _seed;
    }

    [[nodiscard]] UInt32 Generate()
    {
        m_jsr ^= (m_jsr << 17);
        m_jsr ^= (m_jsr >> 13);
        m_jsr ^= (m_jsr << 5);
        return m_jsr;
    }

private:
    UInt32 m_jsr = 0;
};

template<typename Rng>
concept IsRng = requires(Rng _rng) {
    { _rng.Generate() } -> std::same_as<UInt32>;
};

template<IsRng TRng>
[[nodiscard]] float GenerateRandomFloat(
    TRng&       _rng,
    const float _min,
    const float _max)
{
    const UInt32 num    = _rng.Generate();
    const float  rand01 = static_cast<float>(num) / static_cast<float>(Limits<UInt32>::kMax);
    return Lerp(_min, _max, rand01);
}

template<IsRng TRng>
[[nodiscard]] Vec3 GenerateRandomUnitSphere(
    TRng& _rng)
{
    const float rand0  = GenerateRandomFloat(_rng, -1.f, 1.f);
    const float rand1  = GenerateRandomFloat(_rng, -1.f, 1.f) * k2Pi;
    const float sqrtf1 = Sqrt(1.f - rand0 * rand0);
    return { sqrtf1 * Cos(rand1), sqrtf1 * Sin(rand1), rand0 };
}

template<IsRng TRng>
[[nodiscard]] Vec3 GenerateRandomUnitHemisphere(
    TRng&      _rng,
    const Vec3 _normal)
{
    const Vec3  dir   = GenerateRandomUnitSphere(_rng);
    const float dDotn = dir.Dot(_normal);
    return (dDotn < 0.f) ? -dir : dir;
}

}   // namespace Jam