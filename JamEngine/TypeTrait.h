#pragma once

namespace jam
{

namespace detail
{
    template<typename Ty>
    struct TypeName
    {
    private:
        static constexpr std::string_view ExtractTypeName()
        {
            constexpr std::string_view signature { __FUNCSIG__ };
            constexpr std::string_view prefix { "TypeName<" };
            constexpr size_t           startIdx = signature.find(prefix) + prefix.size();
            constexpr size_t           endIdx   = signature.find('>', startIdx);

            static_assert(startIdx != std::string_view::npos, "Failed to find type name in signature");
            static_assert(endIdx != std::string_view::npos, "Failed to find end of type name in signature");
            static_assert(endIdx > startIdx, "End index must be greater than start index");

            std::string_view name = signature.substr(startIdx, endIdx - startIdx);
            if constexpr (!std::is_fundamental_v<Ty>)
            {
                name = name.substr(name.find_last_of(' ') + 1);
            }
            return name;
        }

        static constexpr auto CreateTypeNameBuffer()
        {
            constexpr std::string_view        name = ExtractTypeName();
            std::array<char, name.size() + 1> buffer {};
            std::copy_n(name.data(), name.size(), buffer.data());
            buffer[name.size()] = '\0';
            return buffer;
        }

        static constexpr auto k_typeNameBuffer = CreateTypeNameBuffer();

    public:
        static constexpr std::string_view Value() { return std::string_view { k_typeNameBuffer.data(), k_typeNameBuffer.size() }; }
    };
}   // namespace detail

template<typename T>
NODISCARD constexpr std::string_view NameOf()
{
    return detail::TypeName<T>::Value();
}

template<typename T>
NODISCARD constexpr UInt32 HashOf()
{
    return entt::type_hash<T>::value();
}

}   // namespace jam