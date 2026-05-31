#pragma once

namespace Jam
{

namespace Details
{
    constexpr UInt64 kMurmurSeed = 0xc6a4a7935bd1e995;
}   // namespace Details

class Murmur3Hasher
{
public:
    explicit Murmur3Hasher(UInt64 _seed = Details::kMurmurSeed);
    void                 Mix(MemoryView _block);
    [[nodiscard]] UInt64 Finish() const;

private:
    UInt64 m_hash = 0;
    SizeT  m_len  = 0;
};

}   // namespace Jam