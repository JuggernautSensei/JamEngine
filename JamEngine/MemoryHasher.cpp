#include "pch.h"

#include "MemoryHasher.h"

namespace Jam
{

namespace
{
    [[nodiscard]] UInt64 RotL64_(
        const UInt64 x,
        const int    r)
    {
        return (x << r) | (x >> (64 - r));
    }
}   // namespace

Murmur3Hasher::Murmur3Hasher(
    const UInt64 _seed)
    : m_hash(_seed)
    , m_len(0)
{
}

void Murmur3Hasher::Mix(
    const MemoryView _block)
{
    constexpr UInt64 kC1 = 0x87c37b91114253d5;
    constexpr UInt64 kC2 = 0x4cf5ad432745937f;

    const Byte* pByte      = _block.GetPtr();
    const SizeT byteWidth  = _block.GetByteWidth();
    const SizeT blockCount = byteWidth / 8;
    m_len += byteWidth;

    const UInt64* pBlock = reinterpret_cast<const UInt64*>(pByte);
    for (SizeT i = 0; i < blockCount; ++i)
    {
        UInt64 k1 = pBlock[i];
        k1 *= kC1;
        k1 = RotL64_(k1, 31);
        k1 *= kC2;

        m_hash ^= k1;
        m_hash = RotL64_(m_hash, 27);
        m_hash = m_hash * 5 + 0x52dce729;
    }

    const Byte* pTail = pByte + blockCount * 8;
    UInt64      k1    = 0;

    switch (byteWidth & 7)
    {
        case 7:
            k1 ^= static_cast<UInt64>(pTail[6]) << 48;
            [[fallthrough]];
        case 6:
            k1 ^= static_cast<UInt64>(pTail[5]) << 40;
            [[fallthrough]];
        case 5:
            k1 ^= static_cast<UInt64>(pTail[4]) << 32;
            [[fallthrough]];
        case 4:
            k1 ^= static_cast<UInt64>(pTail[3]) << 24;
            [[fallthrough]];
        case 3:
            k1 ^= static_cast<UInt64>(pTail[2]) << 16;
            [[fallthrough]];
        case 2:
            k1 ^= static_cast<UInt64>(pTail[1]) << 8;
            [[fallthrough]];
        case 1:
            k1 ^= static_cast<UInt64>(pTail[0]);
            k1 *= kC1;
            k1 = RotL64_(k1, 31);
            k1 *= kC2;
            m_hash ^= k1;
        default:
            break;
    }
}

UInt64 Murmur3Hasher::Finish() const
{
    UInt64 h1 = m_hash;
    h1 ^= m_len;
    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccd;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53;
    h1 ^= h1 >> 33;
    return h1;
}

}   // namespace Jam
