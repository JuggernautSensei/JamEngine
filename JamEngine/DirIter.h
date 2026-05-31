#pragma once

namespace Jam
{

// 실제 디렉토리 아이템을 순회할 때 사용하는 반복자 클래스

template<IsCharT CharT>
class DirIterBase
{
public:
    using StringT     = BasicString<CharT>;
    using StringViewT = BasicStringView<CharT>;
    using CharTraits  = CharTraits<CharT>;

private:
    class Impl : public RefCounted
    {
    public:
        [[nodiscard]] static Expect<Ref<Impl>> Create(
            const StringViewT _dir)
        {
            Ref<Impl> pImpl = new Impl;
            if (const Error err = pImpl->Init_(_dir))
            {
                return err;   // failed to initialize the iterator
            }
            return pImpl;
        }

        ~Impl() override
        {
            Close_();
        }

        Impl(const Impl&)            = delete;
        Impl& operator=(const Impl&) = delete;

        Impl(Impl&& other) noexcept
        {
            m_handle       = other.m_handle;
            m_path         = std::move(other.m_path);
            other.m_handle = INVALID_HANDLE_VALUE;
        }

        Impl& operator=(Impl&& other) noexcept
        {
            if (this != &other)
            {
                Close_();
                m_handle       = other.m_handle;
                m_path         = std::move(other.m_path);
                other.m_handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        bool operator==(const Impl& _other) const
        {
            return m_handle == _other.m_handle && m_path == _other.m_path;
        }

        bool operator!=(const Impl& _other) const
        {
            return !(*this == _other);
        }

        void Advance()
        {
            if (m_handle == INVALID_HANDLE_VALUE)
            {
                return;
            }

            WIN32_FIND_DATAW data = {};
            while (::FindNextFileW(m_handle, &data))
            {
                if (!IsUnexpectedFileName_(data.cFileName))
                {
                    m_path = m_path.GetDirectory();
                    m_path.AppendEncode(data.cFileName);
                    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        m_path += CharTraits::kPathPreferredSeparator;
                    }
                    return;
                }
            }

            Close_();   // no more files
        }

        [[nodiscard]] StringViewT GetPath() const
        {
            return m_path;
        }

        [[nodiscard]] bool IsClosed() const
        {
            return m_handle == INVALID_HANDLE_VALUE;
        }

    private:
        Impl() = default;

        Error Init_(
            const StringViewT _dir)
        {
            JAM_ASSERT(_dir.IsDirectory(), "DirIterBase can only be constructed with a directory m_workingDir.");

            WString path = WString::Encoded(_dir);
            path.AppendEncode("*");   // search pattern: all files in the directory

            WIN32_FIND_DATAW data = {};
            m_handle              = ::FindFirstFileW(path.GetPtr(), &data);
            if (m_handle == INVALID_HANDLE_VALUE)
            {
                JAM_LOG_ERROR("Failed to open directory");
                return Error::OpenFailed;
            }

            while (IsUnexpectedFileName_(data.cFileName))
            {
                if (::FindNextFileW(m_handle, &data) == FALSE)
                {
                    Close_();             // no more files
                    return Error::None;   // treat as success since we just reached the end without finding any valid entries
                }
            }

            m_path = _dir;
            m_path.AppendEncode(data.cFileName);
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                m_path += CharTraits::kPathPreferredSeparator;
            }
            return Error::None;
        }

        void Close_()
        {
            if (m_handle != INVALID_HANDLE_VALUE)
            {
                ::FindClose(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
            m_path.Clear();
        }

        bool IsUnexpectedFileName_(
            const WStringView _filename) const
        {
            return _filename == L"." || _filename == L"..";
        }

        HANDLE  m_handle = INVALID_HANDLE_VALUE;
        StringT m_path   = {};
    };

public:
    [[nodiscard]] static Expect<DirIterBase> Create(
        const StringViewT _dir)
    {
        DirIterBase iter = {};
        if (const Error err = iter.Init_(_dir))
        {
            return err;   // failed to initialize the iterator
        }
        return iter;
    }

    DirIterBase() = default;

    [[nodiscard]] StringViewT GetPath() const
    {
        return m_pImpl ? m_pImpl->GetPath() : StringViewT {};
    }

    [[nodiscard]] StringViewT operator*() const
    {
        return GetPath();
    }

    DirIterBase& operator++()
    {
        if (m_pImpl)
        {
            m_pImpl->Advance();
            if (m_pImpl->IsClosed())
            {
                m_pImpl.Reset();   // reached the end, reset to null
            }
        }
        return *this;
    }

    [[nodiscard]] bool operator==(const DirIterBase& _other) const
    {
        return !m_pImpl && !_other.m_pImpl                                   // both are end iterators
            || (m_pImpl && _other.m_pImpl && *m_pImpl == *_other.m_pImpl);   // both point to the same entry
    }

    [[nodiscard]] bool operator!=(const DirIterBase& _other) const
    {
        return !(*this == _other);
    }

private:
    Error Init_(StringViewT _dir)
    {
        Expect<Ref<Impl>> pImpl = Impl::Create(_dir);
        if (!pImpl)
        {
            return pImpl.GetError();   // failed to create the implementation
        }

        if (pImpl.GetValue()->IsClosed())
        {
            return Error::None;   // treat as success since the directory is empty and the iterator is already at the end
        }

        m_pImpl = pImpl.GetValue();
        return Error::None;
    }

    Ref<Impl> m_pImpl = nullptr;
};

template<IsCharT CharT>
[[nodiscard]] DirIterBase<CharT> begin(const DirIterBase<CharT>& _iter)
{
    return _iter;
}

template<IsCharT CharT>
[[nodiscard]] DirIterBase<CharT> end(const DirIterBase<CharT>&)
{
    return {};
}

using DirIter  = DirIterBase<char>;
using DirIterW = DirIterBase<wchar_t>;

}   // namespace Jam