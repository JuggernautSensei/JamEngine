#pragma once

namespace jam
{

class MurmurHash32
{
public:
    explicit MurmurHash32(UInt32 _seed = 0x9747b28c);

    void Mix(const void* _pByte, Int32 _byteWidth);

    template<typename T>
    void Mix(const T& _value)
    {
        Mix(&_value, sizeof(T));
    }

    [[nodiscard]] UInt32 Finish() const;

private:
    UInt32 m_hash   = 0;
    Int32  m_length = 0;
};

class MurmurHash64
{
public:
    explicit MurmurHash64(UInt64 _seed = 0x9747b28c);

    void Mix(const void* _pByte, Int32 _byteWidth);

    template<typename T>
    void Mix(const T& _value)
    {
        Mix(&_value, sizeof(T));
    }

    [[nodiscard]] UInt64 Finish() const;

private:
    UInt64 m_hash   = 0;
    Int32  m_length = 0;
};

[[nodiscard]] UInt32 Hash_Murmur32(const void* _pData, Int32 _byteWidth, UInt32 _seed = 0x9747b28c);
[[nodiscard]] UInt64 Hash_Murmur64(const void* _pData, Int32 _byteWidth, UInt64 _seed = 0x9747b28c);

template<typename T>
[[nodiscard]] UInt32 Hash_Murmur32(const T& _value, const UInt32 _seed = 0x9747b28c)
{
    return Hash_Murmur32(&_value, sizeof(T), _seed);
}

template<typename T>
[[nodiscard]] UInt64 Hash_Murmur64(const T& _value, const UInt64 _seed = 0x9747b28c)
{
    return Hash_Murmur64(&_value, sizeof(T), _seed);
}

}   // namespace jam