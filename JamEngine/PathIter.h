#pragma once

namespace Jam
{

template<IsCharT CharT>
class PathIterBase
{
public:
    using StringT     = BasicString<CharT>;
    using StringViewT = BasicStringView<CharT>;
    using CharTraits  = CharTraits<CharT>;
    using CharFuncs   = CharFuncs<CharT>;

    PathIterBase() = default;

    explicit PathIterBase(
        const StringViewT _path)
        : m_path(_path)
    {
        if (_path.IsEmpty())
        {
            return;
        }

        const CharResult ret = FindPathSeparator(_path);
        m_length             = ret.IsSucceeded() ? ret.pos + 1 : _path.GetLength();   // include the path separator if found
    }

    [[nodiscard]] StringViewT operator*() const
    {
        return GetPath();
    }

    PathIterBase& operator++()
    {
        Advance_();
        return *this;
    }

    [[nodiscard]] bool operator==(const PathIterBase& _other) const
    {
        return (m_length == 0 && _other.m_length == 0)                                                    // both are end iterators
            || (m_path == _other.m_path && m_offset == _other.m_offset && m_length == _other.m_length);   // both point to the same segment
    }

    [[nodiscard]] bool operator!=(const PathIterBase& _other) const
    {
        return !(*this == _other);
    }

    [[nodiscard]] StringViewT GetPath() const
    {
        return m_path.ToStringView(m_offset, m_length);
    }

private:
    void Advance_()
    {
        m_offset += m_length;                 // move to the next segment
        if (m_offset >= m_path.GetLength())   // end of path reached
        {
            m_length = 0;
            return;
        }

        const CharResult ret = FindPathSeparator(m_path.ToStringView(m_offset, m_path.GetLength() - m_offset));
        if (ret.IsSucceeded())
        {
            m_length = ret.pos + 1;   // include the path separator
        }
        else
        {
            m_length = m_path.GetLength() - m_offset;   // last segment
        }
    }

    [[nodiscard]] CharResult FindPathSeparator(StringViewT _path) const
    {
        for (SizeT i = 0; i < _path.GetLength(); ++i)
        {
            if (CharFuncs::IsPathSeparator(_path[i]))
            {
                return CharResult { i };
            }
        }
        return CharResult::Failed();
    }

    StringT m_path   = {};
    SizeT   m_offset = 0;
    SizeT   m_length = 0;
};

template<IsCharT CharT>
[[nodiscard]] PathIterBase<CharT> begin(const PathIterBase<CharT>& _iter)
{
    return _iter;
}

template<IsCharT CharT>
[[nodiscard]] PathIterBase<CharT> end(const PathIterBase<CharT>&)
{
    return {};
}

using PathIter  = PathIterBase<char>;
using WPathIter = PathIterBase<wchar_t>;

}   // namespace Jam