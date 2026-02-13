#pragma once
// ReSharper disable CppUnreachableCode

namespace Jam
{

namespace Details   // Base
{

    struct ExtractResult
    {
        const char* str = 0;
        SizeT       len = 0;
    };

    template<typename T>
    constexpr ExtractResult ExtractFullName()
    {
        // ExtractFullName<...>();
        constexpr StringView raw { JAM_PRETTY_FUNCTION };
        constexpr SizeT      open  = raw.Find("<").pos + 1;
        constexpr SizeT      close = raw.RFind(">").pos;

        SizeT begin = open;
        SizeT end   = close;

        if constexpr (!std::is_fundamental_v<T>)
        {
            // skip prefix (e.g., class, struct, enum)
            for (SizeT i = open; i < end; ++i)
            {
                if (raw[i] == ' ')
                {
                    begin = i + 1;
                    break;
                }
            }
        }

        return ExtractResult { raw.GetPtr() + begin, end - begin };
    }

    template<ExtractResult k_result>
    constexpr auto BuildNameBuffer()
    {
        Array<char, k_result.len + 1> buf = {};
        for (SizeT i = 0; i < k_result.len; ++i)
        {
            buf[i] = k_result.str[i];
        }
        buf[k_result.len] = '\0';
        return buf;
    }

    template<typename T>
    struct TypeNameHolder
    {
        static constexpr auto       k_nameBuf = BuildNameBuffer<ExtractFullName<T>()>();
        static constexpr StringView k_name    = { k_nameBuf.data(), k_nameBuf.size() - 1 };
    };

}   // namespace Details

template<typename T>
[[nodiscard]] constexpr StringView NameOf()
{
    return Details::TypeNameHolder<T>::k_name;
}

// type hash
template<typename T>
[[nodiscard]] constexpr UInt32 HashOf()
{
    constexpr StringView k_name = NameOf<T>();
    constexpr UInt32     k_hash = k_name.Hash32();
    return k_hash;
}

}   // namespace Jam