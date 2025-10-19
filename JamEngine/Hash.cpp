#include "pch.h"

#include "Hash.h"

#include "BaseUtilities.h"

namespace jam
{

static UInt32 RotL32(const UInt32 x, const int r)
{
    return (x << r) | (x >> (32 - r));
}

static UInt64 RotL64(const UInt64 x, const int r)
{
    return (x << r) | (x >> (64 - r));
}

void MurmurHash32::Begin(const UInt32 _seed)
{
    m_hash   = _seed;
    m_length = 0;
}

void MurmurHash32::Add(const void* _pByte, const size_t _byteWidth)
{
    constexpr UInt32 k_c1 = 0xcc9e2d51;
    constexpr UInt32 k_c2 = 0x1b873593;

    const Byte* pByte      = static_cast<const Byte*>(_pByte);
    const Int32 blockCount = NarrowCast<Int32>(_byteWidth / 4);
    m_length += NarrowCast<Int32>(_byteWidth);

    const UInt32* pBlock = reinterpret_cast<const UInt32*>(pByte);
    for (Int32 i = 0; i < blockCount; ++i)
    {
        UInt32 k1 = pBlock[i];
        k1 *= k_c1;
        k1 = RotL32(k1, 15);
        k1 *= k_c2;

        m_hash ^= k1;
        m_hash = RotL32(m_hash, 13);
        m_hash = m_hash * 5 + 0xe6546b64;
    }

    const Byte* pTail = pByte + blockCount * 4;
    UInt32      k1    = 0;

    switch (_byteWidth & 3)
    {
        case 3:
            k1 ^= static_cast<UInt32>(pTail[2]) << 16;
            [[fallthrough]];
        case 2:
            k1 ^= static_cast<UInt32>(pTail[1]) << 8;
            [[fallthrough]];
        case 1:
            k1 ^= static_cast<UInt32>(pTail[0]);
            k1 *= k_c1;
            k1 = RotL32(k1, 15);
            k1 *= k_c2;
            m_hash ^= k1;
    }
}

UInt32 MurmurHash32::End() const
{
    UInt32 h1 = m_hash;
    h1 ^= m_length;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}

void MurmurHash64::Begin(const UInt64 _seed)
{
    m_hash   = _seed;
    m_length = 0;
}

void MurmurHash64::Add(const void* _pByte, const size_t _byteWidth)
{
    constexpr UInt64 k_c1 = 0x87c37b91114253d5;
    constexpr UInt64 k_c2 = 0x4cf5ad432745937f;

    const Byte* pByte      = static_cast<const Byte*>(_pByte);
    const Int32 blockCount = NarrowCast<Int32>(_byteWidth / 8);
    m_length += NarrowCast<Int32>(_byteWidth);

    const UInt64* pBlock = reinterpret_cast<const UInt64*>(pByte);
    for (Int32 i = 0; i < blockCount; ++i)
    {
        UInt64 k1 = pBlock[i];
        k1 *= k_c1;
        k1 = RotL64(k1, 31);
        k1 *= k_c2;

        m_hash ^= k1;
        m_hash = RotL64(m_hash, 27);
        m_hash = m_hash * 5 + 0x52dce729;
    }

    const Byte* pTail = pByte + blockCount * 8;
    UInt64      k1    = 0;

    switch (_byteWidth & 7)
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
            k1 *= k_c1;
            k1 = RotL64(k1, 31);
            k1 *= k_c2;
            m_hash ^= k1;
    }
}

UInt64 MurmurHash64::End() const
{
    UInt64 h1 = m_hash;
    h1 ^= m_length;
    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccd;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53;
    h1 ^= h1 >> 33;
    return h1;
}

}   // namespace jam