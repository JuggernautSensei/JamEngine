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

    template<ExtractResult kResult>
    constexpr auto BuildNameBuffer()
    {
        Array<char, kResult.len + 1> buf = {};
        for (SizeT i = 0; i < kResult.len; ++i)
        {
            buf[i] = kResult.str[i];
        }
        buf[kResult.len] = '\0';
        return buf;
    }

    template<typename T>
    struct TypeNameHolder
    {
        static constexpr auto       kNameBuf = BuildNameBuffer<ExtractFullName<T>()>();
        static constexpr StringView kName    = { kNameBuf.data(), kNameBuf.size() - 1 };
    };

}   // namespace Details

template<typename T>
[[nodiscard]] constexpr StringView NameOf()
{
    return Details::TypeNameHolder<T>::kName;
}

// type hash
template<typename T>
[[nodiscard]] constexpr UInt32 HashOf()
{
    constexpr StringView kName = NameOf<T>();
    constexpr UInt32     kHash = kName.Hash32();
    return kHash;
}

}   // namespace Jam