#pragma once

namespace Jam
{

// 실제 디렉토리 아이템을 순회할 때 사용하는 반복자 클래스
// use like this:
// for (DirIter iter = DirIter::Create("C:/path/to/directory"); iter != DirIter(); ++iter) {
//    StringViewT path = *iter;
//   // do something with path
// }

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
            Ref<Impl> pImpl = new Impl {};
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
            m_dir          = std::move(other.m_dir);
            other.m_handle = INVALID_HANDLE_VALUE;
        }

        Impl& operator=(Impl&& other) noexcept
        {
            if (this != &other)
            {
                Close_();
                m_handle       = other.m_handle;
                m_path         = std::move(other.m_path);
                m_dir          = std::move(other.m_dir);
                other.m_handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        void Next()
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
                    UpdatePath_(data);
                    return;
                }
            }

            // no more file
            Close_();
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

            // search pattern: all files in the directory
            WString path = WString::Encoded(_dir);
            path.AppendEncode("*");

            // open the directory for iteration
            WIN32_FIND_DATAW data = {};
            m_handle              = ::FindFirstFileW(path.GetPtr(), &data);
            if (m_handle == INVALID_HANDLE_VALUE)
            {
                JAM_LOG_ERROR("Failed to open directory");
                return Error::OpenFailed;
            }

            // find first valid file
            while (IsUnexpectedFileName_(data.cFileName))
            {
                if (::FindNextFileW(m_handle, &data) == FALSE)
                {
                    Close_();
                    return Error::None;
                }
            }

            m_dir = _dir;

            // update path
            UpdatePath_(data);
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

        void UpdatePath_(const WIN32_FIND_DATAW& _data)
        {
            m_path = m_dir;
            m_path.AppendEncode(_data.cFileName);
            if (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                m_path += CharTraits::kPreferredSeparator;
            }
        }

        HANDLE  m_handle = INVALID_HANDLE_VALUE;
        StringT m_path   = {};
        StringT m_dir    = {};
    };

public:
    DirIterBase() = default;

    [[nodiscard]] static Expect<DirIterBase> Create(
        const StringViewT _dir)
    {
        DirIterBase iter = {};
        if (const Error err = iter.Init_(_dir))
        {
            return err;
        }
        return iter;
    }

    [[nodiscard]] StringViewT operator*() const
    {
        return m_pImpl ? m_pImpl->GetPath() : StringViewT {};
    }

    DirIterBase& operator++()
    {
        JAM_ASSERT(m_pImpl, "Cannot increment an end iterator.");

        m_pImpl->Next();
        if (m_pImpl->IsClosed())
        {
            m_pImpl.Reset();   // reached the end, reset to null
        }
        return *this;
    }

    [[nodiscard]] bool operator==(const DirIterBase& _other) const
    {
        return m_pImpl == _other.m_pImpl;
    }

    [[nodiscard]] bool operator!=(const DirIterBase& _other) const
    {
        return m_pImpl != _other.m_pImpl;
    }

private:
    Error Init_(
        const StringViewT _dir)
    {
        Expect<Ref<Impl>> pImpl = Impl::Create(_dir);
        if (!pImpl)
        {
            return pImpl.GetError();   // failed to create the implementation
        }

        m_pImpl = pImpl.Take();
        if (m_pImpl->IsClosed())   // empty directory, no files to iterate
        {
            m_pImpl.Reset();
        }

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
