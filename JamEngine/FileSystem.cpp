#include "pch.h"

#include "FileSystem.h"
#include "DirIter.h"
#include "StringSpliter.h"
#include "WindowsUtilities.h"

namespace Jam::FS
{

namespace
{
    [[nodiscard]] UInt64 QuadToUInt64_(
        const UInt32 _high,
        const UInt32 _low)
    {
        return (static_cast<UInt64>(_high) << 32) | _low;
    }

    [[nodiscard]] UInt64 FileTimeToUInt64_(
        const FILETIME _fileTime)
    {
        return QuadToUInt64_(_fileTime.dwHighDateTime, _fileTime.dwLowDateTime);
    }

}   // namespace

namespace Utf16
{

    Error MakeDirectory(
        const WStringView _dir,
        const eOnExists   _onExists)
    {
        JAM_ASSERT(_dir.IsDirectory(), "directory name is must end path seperator");
        JAM_ASSERT(_onExists == eOnExists::Overwrite, "eOnExists::Overwrite is not supported for directory creation");

        if (Exists(_dir))
        {
            switch (_onExists)
            {
                case eOnExists::Fail:
                    JAM_LOG_ERROR(L"Failed to create directory '{}': directory already exists.", _dir);
                    return Error::AlreadyExists;

                case eOnExists::Rename:
                    return MakeDirectory(MakeUniquePath(_dir), eOnExists::Fail);

                case eOnExists::Skip:
                    return Error::None;   // skip creation, treat as successs

                default:
                    JAM_UNREACHABLE;
            }
        }

        // create parent directories
        WString path = {};
        for (const WStringView slice: WStringSpliter { _dir, WString::CharTraits::kPreferredSeparator })
        {
            path.Join(slice);
            if (!Exists(path))
            {
                if (::CreateDirectoryW(path.GetPtr(), NULL) == FALSE)
                {
                    JAM_LOG_ERROR(L"Failed to create directory '{}': {}", slice, MakeWindowsLastErrorMessageW());
                    return ErrorFromWindowsErrorCode(::GetLastError());
                }
            }
        }

        return Error::None;
    }

    bool Exists(
        const WStringView _path)
    {
        const WString path = _path;
        return ::GetFileAttributesW(path.GetPtr()) != INVALID_FILE_ATTRIBUTES;
    }

    bool IsDirectory(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::Directory;
    }

    bool IsFile(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::File;
    }

    bool IsSymbolicLink(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::SymbolicLink;
    }

    Error Remove(
        const WStringView _path)
    {
        if (_path.IsDirectory())
        {
            // remove all entries in the directory
            Expect<DirIterW> iter = DirIterW::Create(_path);
            if (!iter)
            {
                return iter.GetError();   // failed to create directory iterator
            }

            for (const WStringView child: iter.GetValue())
            {
                if (const Error err = Remove(child))
                {
                    return err;   // failed to remove an entry in the directory
                }
            }

            // remove empty directory
            const WString path = _path;
            if (::RemoveDirectoryW(path.GetPtr()) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to remove directory '{}': {}", _path, MakeWindowsLastErrorMessageW());
                return ErrorFromWindowsErrorCode(::GetLastError());
            }
        }
        else
        {
            const WString path = _path;
            if (::DeleteFileW(path.GetPtr()) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to delete file '{}': {}", _path, MakeWindowsLastErrorMessageW());
                return ErrorFromWindowsErrorCode(::GetLastError());
            }
        }

        return Error::None;
    }

    Error Copy(
        const WStringView _srcPath,
        const WStringView _dstPath,
        const eOnExists   _onExists)
    {
        if (_onExists != eOnExists::Overwrite && Exists(_dstPath))
        {
            switch (_onExists)
            {
                case eOnExists::Fail:
                    JAM_LOG_ERROR(L"Failed to copy from '{}' to '{}': destination already exists.", _srcPath, _dstPath);
                    return Error::AlreadyExists;   // fail if destination already exists, treat as error

                case eOnExists::Skip:
                    return Error::None;   // skip copying, treat as success

                case eOnExists::Rename:
                    return Copy(_srcPath, MakeUniquePath(_dstPath), _onExists);

                case eOnExists::Overwrite:
                    break;

                default:
                    JAM_UNREACHABLE;
            }
        }

        if (_dstPath.IsDirectory())
        {
            // create destination directory first
            if (const Error err = MakeDirectory(_dstPath, eOnExists::Fail))
            {
                return err;   // failed to create destination directory
            }

            // copy all entries in the source directory
            Expect<DirIterW> iter = DirIterW::Create(_srcPath);
            if (!iter)
            {
                return iter.GetError();   // failed to create directory iterator
            }

            for (const WStringView childSrc: iter.GetValue())
            {
                WString childDst = _dstPath;
                childDst.Join(childSrc.GetFileName(), childSrc.IsDirectory() ? eJoin::Directory : eJoin::File);
                if (const Error err = Copy(childSrc, childDst, _onExists))
                {
                    return err;   // failed to copy an entry in the directory
                }
            }
        }
        else
        {
            const WString srcPath = _srcPath;
            const WString dstPath = _dstPath;
            if (::CopyFileW(srcPath.GetPtr(), dstPath.GetPtr(), FALSE) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to copy file from '{}' to '{}': {}", _srcPath, _dstPath, MakeWindowsLastErrorMessageW());
                return ErrorFromWindowsErrorCode(::GetLastError());
            }
        }

        return Error::None;
    }

    Error Rename(
        const WStringView _srcPath,
        const WStringView _dstPath,
        const eOnExists   _onExists)
    {
        if (_onExists != eOnExists::Overwrite && Exists(_dstPath))
        {
            switch (_onExists)
            {
                case eOnExists::Fail:
                    JAM_LOG_ERROR(L"Failed to rename from '{}' to '{}': destination already exists.", _srcPath, _dstPath);
                    return Error::AlreadyExists;   // fail if destination already exists, treat as error

                case eOnExists::Skip:
                    return Error::None;   // skip renaming, treat as success

                case eOnExists::Rename:
                    return Rename(_srcPath, MakeUniquePath(_dstPath), _onExists);

                case eOnExists::Overwrite:
                default:
                    JAM_UNREACHABLE;
            }
        }

        DWORD flags = MOVEFILE_COPY_ALLOWED;
        if (_onExists == eOnExists::Overwrite)
        {
            flags |= MOVEFILE_REPLACE_EXISTING;
        }

        const WString srcPath = _srcPath;
        const WString dstPath = _dstPath;
        if (::MoveFileExW(srcPath.GetPtr(), dstPath.GetPtr(), flags) == FALSE)
        {
            JAM_LOG_ERROR(L"Failed to rename from '{}' to '{}': {}", _srcPath, _dstPath, MakeWindowsLastErrorMessageW());
            return ErrorFromWindowsErrorCode(::GetLastError());
        }

        return Error::None;
    }

    UInt64 QueryFileSize(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).fileSize;
    }

    UInt64 QueryLastWriteTime(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).lastWriteTime;
    }

    FileAttribute QueryFileAttribute(
        const WStringView _path)
    {
        const WString             path = _path;
        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (::GetFileAttributesExW(path.GetPtr(), GetFileExInfoStandard, &data) == FALSE)
        {
            JAM_ASSERT(0, "Failed to query file attributes");
        }

        FileAttribute attrib = {};
        attrib.type          = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY     ? eFile::Directory
                             : data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? eFile::SymbolicLink
                                                                                    : eFile::File;

        attrib.creationTime   = FileTimeToUInt64_(data.ftCreationTime);
        attrib.lastAccessTime = FileTimeToUInt64_(data.ftLastAccessTime);
        attrib.lastWriteTime  = FileTimeToUInt64_(data.ftLastWriteTime);
        attrib.fileSize       = QuadToUInt64_(data.nFileSizeHigh, data.nFileSizeLow);
        return attrib;
    }

    bool IsEmpty(
        const WStringView _path)
    {
        if (_path.IsDirectory())
        {
            Expect<DirIterW> iter = DirIterW::Create(_path);
            if (!iter)
            {
                JAM_LOG_ERROR(L"Failed to create directory iterator for path '{}'. {}", _path, iter.GetError());
                return false;   // treat as not empty if failed to create directory iterator
            }

            return iter.GetValue() == DirIterW {};
        }

        return QueryFileSize(_path) == 0;
    }

    WString Relative(
        const WStringView _from,
        const WStringView _to)
    {
        const WString fromAbs = Absolute(_from);
        const WString toAbs   = Absolute(_to);

        WStringSpliter fromSpliter { fromAbs, WString::CharTraits::kPreferredSeparator };
        WStringSpliter toSpliter { toAbs, WString::CharTraits::kPreferredSeparator };

        while (!fromSpliter.IsDone() && !toSpliter.IsDone() && fromSpliter.Get() == toSpliter.Get())
        {
            fromSpliter.Next();
            toSpliter.Next();
        }

        if (fromSpliter.IsDone() && toSpliter.IsDone())
        {
            return L".";   // both paths are the same
        }

        Int32 upLevels = 0;
        for (const WStringView slice: fromSpliter)
        {
            if (!slice.IsEmpty() && slice != L"." && slice != L"..")
            {
                ++upLevels;
            }
            else if (slice == L"..")
            {
                --upLevels;   // ".." cancels out a previous level
            }
        }

        if (fromAbs.IsFile() && upLevels > 0)
        {
            --upLevels;   // the last segment of a file path is not a directory level
        }

        if (upLevels < 0)
        {
            return {};
        }

        WString result = {};
        result.Reserve(upLevels * 3 + toAbs.GetLength());   // reserve enough capacity for the result
        for (Int32 i = 0; i < upLevels; ++i)
        {
            result.Join(L"..", eJoin::Directory);
        }

        for (const WStringView slice: toSpliter)
        {
            result.Join(slice);
        }

        // result.Simplify(); maybe not necessary since both fromAbs and toAbs are already simplified by Absolute()
        return result;
    }

    WString Absolute(
        const WStringView _path)
    {
        if (_path.IsAbsolute())
        {
            WString path = _path;
            path.Simplify();
            return path;
        }

        const WString path = _path;
        SizeT         len  = ::GetFullPathNameW(path.GetPtr(), 0, nullptr, nullptr);

        WString absolute = {};
        absolute.ResizeUninitialized(len);
        ::GetFullPathNameW(path.GetPtr(), static_cast<DWORD>(absolute.GetLength()), absolute.GetPtr(), nullptr);

        if (path.IsDirectory())
        {
            absolute.Append(WString::CharTraits::kPreferredSeparator);
        }

        absolute.Simplify();
        return absolute;
    }

    WString Current()
    {
        const SizeT len = ::GetCurrentDirectoryW(0, nullptr);   // + separator for directory

        WString current = {};
        current.ResizeUninitialized(len);
        ::GetCurrentDirectoryW(static_cast<DWORD>(current.GetLength()), current.GetPtr());
        current.Append(WString::CharTraits::kPreferredSeparator);

        current.Simplify();
        return current;
    }

    WString Executable()
    {
        const SizeT len = ::GetModuleFileNameW(NULL, nullptr, 0);

        WString executable = {};
        executable.ResizeUninitialized(len);
        ::GetModuleFileNameW(NULL, executable.GetPtr(), static_cast<DWORD>(executable.GetLength()));

        executable.Simplify();
        return executable;
    }

    WString MakeUniquePath(
        const WStringView _basePath)
    {
        if (!Exists(_basePath))
        {
            return _basePath;   // base path does not exist, treat as unique
        }

        WStringView dir = {}, base = {}, ext = {};
        _basePath.Decompose(&dir, nullptr, &base, &ext);

        WString result = {};
        result.Reserve(_basePath.GetLength() + 10);   // reserve enough capacity for the result
        for (Int32 counter = 1;; ++counter)
        {
            const wchar_t dirSep = base.IsDirectory() ? WString::CharTraits::kPreferredSeparator : L' ';
            result.Format(L"{}{} ({}){}{}", dir, base, counter, ext, dirSep);

            if (!Exists(result))
            {
                break;   // found a unique path
            }
        }
        return result;
    }

    bool ComparePathEqual(
        const WStringView _path0,
        const WStringView _path1)
    {
        return Absolute(_path0).CompareEqualIgnoreCase(Absolute(_path1));
    }

}   // namespace Utf16

namespace Utf8
{

    Error MakeDirectory(
        const StringView _dir,
        const eOnExists  _onExists)
    {
        const WString path = WString::Encoded(_dir);
        return Utf16::MakeDirectory(path, _onExists);
    }

    bool Exists(
        const StringView _path)
    {
        const WString path = WString::Encoded(_path);
        return Utf16::Exists(path);
    }

    bool IsDirectory(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::Directory;
    }

    bool IsFile(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::File;
    }

    bool IsSymbolicLink(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFile::SymbolicLink;
    }

    Error Remove(
        const StringView _path)
    {
        const WString path = WString::Encoded(_path);
        return Utf16::Remove(path);
    }

    Error Copy(
        const StringView _srcPath,
        const StringView _dstPath,
        const eOnExists  _onExists)
    {
        const WString srcPath = WString::Encoded(_srcPath);
        const WString dstPath = WString::Encoded(_dstPath);
        return Utf16::Copy(srcPath, dstPath, _onExists);
    }

    Error Rename(
        const StringView _srcPath,
        const StringView _dstPath,
        const eOnExists  _onExists)
    {
        WString srcPath;
        srcPath.Encode(_srcPath);

        WString dstPath;
        dstPath.Encode(_dstPath);

        return Utf16::Rename(srcPath, dstPath, _onExists);
    }

    UInt64 QueryFileSize(
        const StringView _path)
    {
        return QueryFileAttribute(_path).fileSize;
    }

    UInt64 QueryLastWriteTime(
        const StringView _path)
    {
        return QueryFileAttribute(_path).lastWriteTime;
    }

    FileAttribute QueryFileAttribute(
        const StringView _path)
    {
        const WString path = WString::Encoded(_path);
        return Utf16::QueryFileAttribute(path);
    }

    bool IsEmpty(
        const StringView _path)
    {
        if (_path.IsDirectory())
        {
            Expect<DirIter> iter = DirIter::Create(_path);
            if (!iter)
            {
                JAM_LOG_ERROR("Failed to create directory iterator for path '{}'. {}", _path, iter.GetError());
                return false;   // treat as not empty if failed to create directory iterator
            }

            return iter.GetValue() == DirIter {};
        }

        return QueryFileSize(_path) == 0;
    }

    String Relative(
        const StringView _from,
        const StringView _to)
    {
        const WString from = WString::Encoded(_from);
        const WString to   = WString::Encoded(_to);
        const WString rel  = Utf16::Relative(from, to);
        return String::Encoded(rel.ToStringView());
    }

    String Absolute(
        const StringView _path)
    {
        if (_path.IsAbsolute())
        {
            String path = _path;
            path.Simplify();
            return path;
        }

        const WString path = WString::Encoded(_path);
        const WString abs  = Utf16::Absolute(path);
        return String::Encoded(abs.ToStringView());
    }

    String Current()
    {
        const WString current = Utf16::Current();
        return String::Encoded(current.ToStringView());
    }

    String Executable()
    {
        const WString executable = Utf16::Executable();
        return String::Encoded(executable.ToStringView());
    }

    String MakeUniquePath(
        const StringView _basePath)
    {
        const WString basePath   = WString::Encoded(_basePath);
        const WString uniquePath = Utf16::MakeUniquePath(basePath);
        return String::Encoded(uniquePath.ToStringView());
    }

    bool ComparePathEqual(
        const StringView _path0,
        const StringView _path1)
    {
        const WString path0 = WString::Encoded(_path0);
        const WString path1 = WString::Encoded(_path1);
        return Utf16::ComparePathEqual(path0, path1);
    }

}   // namespace Utf8

}   // namespace Jam::FS