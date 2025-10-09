#pragma once

namespace jam
{

namespace details
{
    template<typename Ty>
    struct TypeName
    {
    private:
        static constexpr std::string_view ExtractTypeName()
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

        static constexpr auto k_typeNameBuf = CreateTypeNameBuffer();

    public:
        static constexpr std::string_view Value()
        {
            return std::string_view { k_typeNameBuf.data(), k_typeNameBuf.size() };
        }
    };
}   // namespace internal

template<typename T>
JAM_NODISCARD constexpr std::string_view NameOf()
{
    return details::TypeName<T>::Value();
}

template<typename T>
JAM_NODISCARD constexpr UInt32 HashOf()
{
    return entt::type_hash<T>::value();
}

template<typename T>
constexpr bool AlwaysFalse = false;

template<typename T>
constexpr bool IsStdContainer = std::is_class_v<T> && requires(T a) {
    a.begin();
    a.end();
    a.size();
};

template<typename T>
constexpr bool IsStdContiguousSequenceContainer = IsStdContainer<T> && std::ranges::contiguous_range<T>;

template<typename T>
constexpr bool IsPlainOfData = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;


}   // namespace jam