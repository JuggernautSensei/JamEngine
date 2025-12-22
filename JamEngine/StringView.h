#pragma once

namespace jam
{

// null terminator를 보장하는 string view 클래스
// 수정 불가, 소유하지 않음, 단순 읽기전용 뷰
// gls::czstring_view와 유사. std::string_view와는 다름
// 안전한? 사용성이 좋은? StringLiteral 대체재로 사용 가능
template<typename T>
class BasicStringView
{
public:
    static_assert(std::is_same_v<T, char> || std::is_same_v<T, wchar_t>);
    using CharTrait  = std::char_traits<T>;
    using StringType = std::basic_string<T>;

    constexpr BasicStringView()
    {
    }

    template<Int32 N>
    /* implicit */ constexpr BasicStringView(const T (&_array)[N])
        : m_str(_array)
        , m_len(N - 1)   // exclude null-terminator
    {
    }

    // _len is the length excluding null-terminator
    /* implicit */ constexpr BasicStringView(const T* _str, const Int32 _len = -1)
        : m_str(_str)
        , m_len(_len)
    {
        if (m_str)
        {
            if (_len < 0)
            {
                m_len = NarrowCast<Int32>(std::char_traits<T>::length(m_str));
            }
            else
            {
                // check null-termination within the given length
                JAM_ASSERT(IsNullTerminated(), "String is not null-terminated within the given length");
            }
        }
    }

    /* implicit */ BasicStringView(const StringType& _str)
        : m_str(_str.c_str())
        , m_len(NarrowCast<Int32>(_str.size()))
    {
    }

    [[nodiscard]] constexpr const T* GetPtr() const
    {
        return m_str;
    }

    [[nodiscard]] constexpr StringType ToString() const
    {
        return std::string { m_str, m_len };
    }

    [[nodiscard]] constexpr StringView SubView(const Int32 _start) const
    {
        // StringView must guarantee null-termination
        // so, we cannot create a subview that goes beyond the original null-terminator
        JAM_ASSERT(_start >= 0 && _start <= m_len, "Start index out of range");
        return BasicStringView { m_str + _start, m_len - _start };
    }

    [[nodiscard]] constexpr Int32 GetLength() const
    {
        return m_len;
    }

    constexpr void Clear()
    {
        m_str = nullptr;
        m_len = 0;
    }

    [[nodiscard]] constexpr bool IsEmpty() const
    {
        return m_len == 0;
    }

    [[nodiscard]] Int32 FindFirstOf(const T _ch) const   // reurns -1 if not found
    {
        if (m_str == nullptr)
        {
            return -1;
        }

        const T* c = CharTrait::find(m_str, m_len, _ch);
        if (c)
        {
            return (c - m_str) / sizeof(CharTrait::char_type);
        }
        else
        {
            return -1;
        }
    }

    [[nodiscard]] constexpr const T* begin() const
    {
        return m_str;
    }

    [[nodiscard]] constexpr const T* end() const
    {
        return m_str ? m_str + m_len : nullptr;
    }

    [[nodiscard]] constexpr const T& operator[](const Int32 _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < m_len, "Index out of range");
        return m_str[_index];
    }

    [[nodiscard]] constexpr const T& At(const Int32 _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < m_len, "Index out of range");
        return m_str[_index];
    }

    [[nodiscard]] constexpr bool operator==(const BasicStringView& _other) const
    {
        if (m_len != _other.m_len)
        {
            return false;
        }

        return std::char_traits<T>::compare(m_str, _other.m_str, m_len) == 0;
    }

    [[nodiscard]] constexpr bool operator!=(const BasicStringView& _other) const
    {
        return !(*this == _other);
    }

private:
    [[nodiscard]] constexpr bool IsNullTerminated() const
    {
        if (m_str == nullptr)
        {
            return true;
        }
        else
        {
            return m_str[m_len] == T('\0');
        }
    }

    const T* m_str = nullptr;
    Int32    m_len = 0;
};

}   // namespace jam

template<>
struct std::formatter<jam::BasicStringView<char>> : std::formatter<std::string_view>
{
    auto format(const jam::BasicStringView<char> p, format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(
            std::string_view { p.GetPtr(), static_cast<size_t>(p.GetLength()) }, ctx);
    }
};
