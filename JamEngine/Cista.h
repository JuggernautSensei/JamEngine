#pragma once

#include <cista/serialization.h>

namespace jam
{

template<typename T, size_t N>
using CistaArray = cista::offset::array<T, N>;

template<typename T>
using CistaVector = cista::offset::vector<T>;

using CistaString = cista::offset::string;

template<typename T, cista::mode Mode = cista::mode::NONE>
[[nodiscard]] Optional<T* const> DeserializeCista(
    const CMemView _mem)
{
    try
    {
        T* const deserialized = cista::deserialize<T>(_mem.GetPtr());
        return deserialized;
    }
    catch (const std::exception& _e)
    {
        GlobalBuffer ::SetLastError("Cista deserialization failed: {}", _e.what());
        return k_nullopt;
    }
}

template<typename T, cista::mode Mode = cista::mode::NONE>
Optional<Vector<Byte>> SerializeCista(
    const T& _data)
{
    try
    {
        cista::buf<Vector<Byte>> serialized = {};
        return cista::serialize<Mode>(serialized, _data);
    }
    catch (const std::exception& _e)
    {
        GlobalBuffer::SetLastError("Cista serialization failed: {}", _e.what());
        return k_nullopt;
    }
}

}   // namespace jam