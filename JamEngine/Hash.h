#pragma once

namespace jam
{

class MurmurHash32
{
public:
    void                 Begin(UInt32 _seed = 0x9747b28c);
    void                 Add(const void* _pByte, size_t _byteWidth);
    [[nodiscard]] UInt32 End() const;

private:
    UInt32 m_hash   = 0;
    Int32  m_length = 0;
};

class MurmurHash64
{
public:
    void                 Begin(UInt64 _seed = 0x9747b28c);
    void                 Add(const void* _pByte, size_t _byteWidth);
    [[nodiscard]] UInt64 End() const;

private:
    UInt64 m_hash   = 0;
    Int32  m_length = 0;
};

[[nodiscard]] UInt32 Hash_Murmur32(const void* _pData, size_t _byteWidth, UInt32 _seed = 0x9747b28c);
[[nodiscard]] UInt64 Hash_Murmur64(const void* _pData, size_t _byteWidth, UInt64 _seed = 0x9747b28c);

}   // namespace jam