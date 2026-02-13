#pragma once
// ReSharper disable CppUninitializedNonStaticDataMember
// ReSharper disable CppClassNeedsConstructorBecauseOfUninitializedMember

namespace Jam
{

// A simple scratch buffer with a fixed capacity for temporary string formatting and manipulation.
// use like this:
//      ScratchBuffer sb;   <- do not zero-initialize. this is zero-overhead.
//      sb.Format("Hello, {}!", "World");
//      printf("%s\n", sb.GetPtr());
//
// do not use like this:
//      ScratchBuffer sb = {}; <- this will be zero-initialized, causing unnecessary overhead.
//

template<SizeT k_cap = 512>
class BasicScratchBuffer
{
    static_assert(k_cap > 0, "Capacity must be greater than zero.");

public:
    template<typename... TArgs>
    void Format(
        fmt::format_string<TArgs...> _fmt,
        TArgs&&... _args)
    {
        const auto [end, _] = fmt::format_to_n(m_buf, k_cap - 1, _fmt, std::forward<TArgs>(_args)...);
        m_len               = end - m_buf;
        *end                = '\0';
    }

    template<typename... TArgs>
    void AppendFormat(
        fmt::format_string<TArgs...> _fmt,
        TArgs&&... _args)   // do not call before Format() or Set(). this is not safe.
    {
        const auto [end, _] = fmt::format_to_n(m_buf + m_len, k_cap - 1 - m_len, _fmt, std::forward<TArgs>(_args)...);
        m_len               = end - m_buf;
        *end                = '\0';
    }

    void Set(
        const StringView _str)
    {
        const SizeT len = Min(_str.GetLength(), k_cap - 1);
        std::memcpy(m_buf, _str.GetPtr(), len);
        m_len        = len;
        m_buf[m_len] = '\0';
    }

    void Append(
        const StringView _str)   // do not call before Format() or Set(). this is not safe.
    {
        const SizeT len = Min(_str.GetLength(), k_cap - 1 - m_len);
        if (len > 0)
        {
            std::memcpy(m_buf + m_len, _str.GetPtr(), len);
            m_len += len;
            m_buf[m_len] = '\0';
        }
    }

    [[nodiscard]] const char* GetPtr() const
    {
        return m_buf;
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return k_cap;
    }

    [[nodiscard]] StringView ToStringView() const
    {
        return { m_buf, m_len };
    }

    [[nodiscard]] String ToString() const
    {
        return { m_buf, m_len };
    }

    [[nodiscard]] std::string_view ToStdStringView() const
    {
        return { m_buf, NumericCast<USizeT>(m_len) };
    }

    [[nodiscard]] std::string ToStdString() const
    {
        return { m_buf, NumericCast<USizeT>(m_len) };
    }

private:
    char  m_buf[k_cap];
    SizeT m_len;
};

using ScratchBuffer = BasicScratchBuffer<>;
static_assert(std::is_trivial_v<BasicScratchBuffer<>>, "BasicScratchBuffer must be a trivial type.");

template<SizeT k_cap = 512>
class BasicScratchWBuffer
{
public:
    template<typename... TArgs>
    void Format(
        fmt::wformat_string<TArgs...> _fmt,
        TArgs&&... _args)
    {
        const auto [end, _] = fmt::format_to_n(m_buf, k_cap - 1, _fmt, std::forward<TArgs>(_args)...);
        m_len               = end - m_buf;
        *end                = L'\0';
    }

    template<typename... TArgs>
    void AppendFormat(
        fmt::wformat_string<TArgs...> _fmt,
        TArgs&&... _args)   // do not call before Format() or Set(). this is not safe.
    {
        const auto [end, _] = fmt::format_to_n(m_buf + m_len, k_cap - 1 - m_len, _fmt, std::forward<TArgs>(_args)...);
        m_len               = end - m_buf;
        *end                = L'\0';
    }

    void Set(
        const std::wstring_view _str)
    {
        const SizeT len = Min(_str.length(), k_cap - 1);
        std::memcpy(m_buf, _str.data(), len * sizeof(wchar_t));
        m_len        = len;
        m_buf[m_len] = L'\0';
    }

    void Append(
        const std::wstring_view _str)   // do not call before Format() or Set(). this is not safe.
    {
        const SizeT len = Min(_str.length(), k_cap - 1 - m_len);
        if (len > 0)
        {
            std::memcpy(m_buf + m_len, _str.data(), len * sizeof(wchar_t));
            m_len += len;
            m_buf[m_len] = L'\0';
        }
    }

    void SetUtf8(
        const StringView _str)
    {
        m_len = ConvertToUtf16(_str.GetPtr(), _str.GetTerminate(), m_buf, k_cap);
    }

    void AppendUtf8(
        const StringView _str)   // do not call before Format() or Set(). this is not safe.
    {
        const SizeT written = ConvertToUtf16(_str.GetPtr(), _str.GetTerminate(), m_buf + m_len, k_cap - m_len);
        m_len += written;
    }

    [[nodiscard]] const wchar_t* GetPtr() const
    {
        return m_buf;
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] constexpr SizeT GetCapacity() const
    {
        return k_cap;
    }

    [[nodiscard]] std::wstring_view ToStdStringView() const
    {
        return { m_buf, NumericCast<USizeT>(m_len) };
    }

    [[nodiscard]] std::wstring ToStdString() const
    {
        return { m_buf, NumericCast<USizeT>(m_len) };
    }

private:
    wchar_t m_buf[k_cap];
    SizeT   m_len;
};
using ScratchWBuffer = BasicScratchWBuffer<>;
static_assert(std::is_trivial_v<BasicScratchWBuffer<>>, "BasicScratchBuffer must be a trivial type.");

}   // namespace Jam