#pragma once

namespace jam
{

// 비암호학 32bit 해시 함수
// murmurhash3 기반
class MurmurHash32
{
public:
    void                 Begin(UInt32 _seed = 0x9747b28c);
    void                 Add(const void* _pByte, size_t _byteWidth);
    JAM_NODISCARD UInt32 End() const;

private:
    UInt32 m_hash   = 0;
    Int32  m_length = 0;
};

// 비암호학 64bit 해시 함수
// murmurhash3 기반
class MurmurHash64
{
public:
    void                 Begin(UInt64 _seed = 0x9747b28c);
    void                 Add(const void* _pByte, size_t _byteWidth);
    JAM_NODISCARD UInt64 End() const;

private:
    UInt64 m_hash   = 0;
    Int32  m_length = 0;
};

}   // namespace jam