#pragma once

namespace jam
{

namespace details
{
    template<typename Ty>
    struct TypeName
    {
    private:
        static constexpr std::string_view ExtractName()
        {
            constexpr std::string_view signature { JAM_PRETTY_FUNCTION };
            constexpr std::string_view prefix { "TypeName<" };
            constexpr size_t           startIdx = signature.find(prefix) + prefix.size();
            constexpr size_t           endIdx   = signature.find('>', startIdx);

            static_assert(startIdx != std::string_view::npos, "Failed to find type label in signature");
            static_assert(endIdx != std::string_view::npos, "Failed to find end of type label in signature");
            static_assert(endIdx > startIdx, "End index must be greater than start index");

            std::string_view name = signature.substr(startIdx, endIdx - startIdx);
            if constexpr (!std::is_fundamental_v<Ty>)
            {
                name = name.substr(name.find_last_of(' ') + 1);
            }
            return name;   // typename excluding null-terminator
        }

        static constexpr auto CreateNameBuffer()
        {
            constexpr std::string_view        name   = ExtractName();
            std::array<char, name.size() + 1> buffer = {};
            std::copy_n(name.data(), name.size(), buffer.data());
            buffer[name.size()] = '\0';
            return buffer;   // buffer including null-terminator
        }

        static constexpr auto k_nameBuffer = CreateNameBuffer();

    public:
        static constexpr StringView Value()
        {
            return StringView { k_nameBuffer.data(), static_cast<Int32>(k_nameBuffer.size() - 1) };
        }
    };
}   // namespace details

template<typename T>
[[nodiscard]] constexpr StringView NameOf()
{
    return details::TypeName<T>::Value();
}

[[nodiscard]] constexpr UInt32 HashOf(const StringView _str)
{
    return entt::hashed_string { _str.GetPtr(), static_cast<size_t>(_str.GetLength()) }.value();
}

[[nodiscard]] constexpr UInt32 HashOf(const WStringView _wStr)
{
    return entt::hashed_wstring { _wStr.GetPtr(), static_cast<size_t>(_wStr.GetLength()) }.value();
}

[[nodiscard]] std::string ToLower(StringView _str);

[[nodiscard]] Optional<std::string>  ToUtf8(WStringView _wstr);
[[nodiscard]] Optional<std::wstring> ToUtf16(StringView _str);

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] Optional<T> IntFrom(const StringView _str)
{
    T value;

    const std::from_chars_result result = std::from_chars(_str.GetPtr(), _str.GetPtr() + _str.GetLength(), value);
    if (result.ec == std::errc())
    {
        return value;
    }
    else
    {
        return k_nullopt;
    }
}

}   // namespace jam