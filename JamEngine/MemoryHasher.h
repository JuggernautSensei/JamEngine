#pragma once

namespace Jam
{

namespace Details
{
    constexpr UInt32 kMurmur32Seed = 0x9747b28c;
    constexpr UInt64 kMurmur64Seed = 0xc6a4a7935bd1e995;
}   // namespace Details

class Murmur3Hasher32
{
public:
    explicit Murmur3Hasher32(UInt32 _seed = Details::kMurmur32Seed);
    void Mix(MemoryView _block);
    [[nodiscard]] UInt32 Finish() const;

private:
    UInt32 m_hash = 0;
    SizeT  m_len  = 0;
};

class Murmur3Hasher64
{
public:
    explicit Murmur3Hasher64(UInt64 _seed = Details::kMurmur64Seed);
    void Mix(MemoryView _block);
    [[nodiscard]] UInt64 Finish() const;

private:
    UInt64 m_hash = 0;
    SizeT  m_len  = 0;
};

}   // namespace Jam