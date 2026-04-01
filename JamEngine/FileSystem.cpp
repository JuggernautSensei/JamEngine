#include "pch.h"
#include "FileSystem.h"

#include "DirIter.h"
#include "PathIter.h"
#include "WindowsUtilities.h"

namespace Jam::FileSystem
{

namespace
{
    [[nodiscard]] UInt64 QuadToUInt64_(
        const UInt32 _high,
        const UInt32 _low)
    {
        return (NumericCast<UInt64>(_high) << 32) | _low;
    }

    [[nodiscard]] UInt64 FileTimeToUInt64_(
        const FILETIME _fileTime)
    {
        return QuadToUInt64_(_fileTime.dwHighDateTime, _fileTime.dwLowDateTime);
    }

}   // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Utf16
{

    Error MakeDirectory(
        const WStringView                   _dir,
        const eMakeDirectoryCollisionPolicy _policy)
    {
        JAM_ASSERT(_dir.IsDirectory(), "directory name is must end path seperator");

        if (Exists(_dir))
        {
            switch (_policy)
            {
                case eMakeDirectoryCollisionPolicy::Fail:
                    JAM_LOG_ERROR(L"Failed to create directory '{}': directory already exists.", _dir);
                    return Error::AlreadyExists;

                case eMakeDirectoryCollisionPolicy::Rename:
                    return MakeDirectory(GenerateUniquePath(_dir), eMakeDirectoryCollisionPolicy::Fail);

                case eMakeDirectoryCollisionPolicy::Skip:
                    return Error::None;   // skip creation, treat as success

                default:
                    JAM_UNREACHABLE;
            }
        }

        // create parent directories
        PathStackWString path;
        for (const WStringView slice: PathIterW { _dir })
        {
            path.Join(slice);
            if (!Exists(path))
            {
                if (::CreateDirectoryW(path.GetPtr(), NULL) == FALSE)
                {
                    JAM_LOG_ERROR(L"Failed to create directory '{}': {}", slice, CreateWindowsLastErrorMessageW());
                    return ErrorFromPlatformErrorCode(::GetLastError());
                }
            }
        }

        return Error::None;
    }

    bool Exists(
        const WStringView _path)
    {
        const PathStackWString path = _path;
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
            const PathStackWString path = _path;
            if (::RemoveDirectoryW(path.GetPtr()) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to remove directory '{}': {}", _path, CreateWindowsLastErrorMessageW());
                return ErrorFromPlatformErrorCode(::GetLastError());
            }
        }
        else
        {
            const PathStackWString path = _path;
            if (::DeleteFileW(path.GetPtr()) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to delete file '{}': {}", _path, CreateWindowsLastErrorMessageW());
                return ErrorFromPlatformErrorCode(::GetLastError());
            }
        }

        return Error::None;
    }

    Error Copy(
        const WStringView      _srcPath,
        const WStringView      _dstPath,
        const eCollisionPolicy _policy)
    {
        if (_policy != eCollisionPolicy::Overwrite && Exists(_dstPath))
        {
            switch (_policy)
            {
                case eCollisionPolicy::Fail:
                    JAM_LOG_ERROR(L"Failed to copy from '{}' to '{}': destination already exists.", _srcPath, _dstPath);
                    return Error::AlreadyExists;   // fail if destination already exists, treat as error

                case eCollisionPolicy::Skip:
                    return Error::None;   // skip copying, treat as success

                case eCollisionPolicy::Rename:
                    return Copy(_srcPath, GenerateUniquePath(_dstPath), _policy);

                case eCollisionPolicy::Overwrite:
                    break;

                default:
                    JAM_UNREACHABLE;
            }
        }

        if (_dstPath.IsDirectory())
        {
            // create destination directory first
            if (const Error err = MakeDirectory(_dstPath, eMakeDirectoryCollisionPolicy::Fail))
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
                PathStackWString childDst = _dstPath;
                childDst.Join(childSrc.GetFileName(), childSrc.IsDirectory() ? eJoin::Directory : eJoin::File);
                if (const Error err = Copy(childSrc, childDst, _policy))
                {
                    return err;   // failed to copy an entry in the directory
                }
            }
        }
        else
        {
            const PathStackWString srcPath = _srcPath;
            const PathStackWString dstPath = _dstPath;
            if (::CopyFileW(srcPath.GetPtr(), dstPath.GetPtr(), FALSE) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to copy file from '{}' to '{}': {}", _srcPath, _dstPath, CreateWindowsLastErrorMessageW());
                return ErrorFromPlatformErrorCode(::GetLastError());
            }
        }

        return Error::None;
    }

    Error Rename(
        const WStringView      _srcPath,
        const WStringView      _dstPath,
        const eCollisionPolicy _policy)
    {
        if (_policy != eCollisionPolicy::Overwrite && Exists(_dstPath))
        {
            switch (_policy)
            {
                case eCollisionPolicy::Fail:
                    JAM_LOG_ERROR(L"Failed to rename from '{}' to '{}': destination already exists.", _srcPath, _dstPath);
                    return Error::AlreadyExists;   // fail if destination already exists, treat as error

                case eCollisionPolicy::Skip:
                    return Error::None;   // skip renaming, treat as success

                case eCollisionPolicy::Rename:
                    return Rename(_srcPath, GenerateUniquePath(_dstPath), _policy);

                case eCollisionPolicy::Overwrite:
                default:
                    JAM_UNREACHABLE;
            }
        }

        DWORD flags = MOVEFILE_COPY_ALLOWED;
        if (_policy == eCollisionPolicy::Overwrite)
        {
            flags |= MOVEFILE_REPLACE_EXISTING;
        }

        const PathStackWString srcPath = _srcPath;
        const PathStackWString dstPath = _dstPath;
        if (::MoveFileExW(srcPath.GetPtr(), dstPath.GetPtr(), flags) == FALSE)
        {
            JAM_LOG_ERROR(L"Failed to rename from '{}' to '{}': {}", _srcPath, _dstPath, CreateWindowsLastErrorMessageW());
            return ErrorFromPlatformErrorCode(::GetLastError());
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
        const PathStackWString    path = _path;
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
                JAM_LOG_ERROR(L"Failed to create directory iterator for path '{}': {}", _path, iter.GetError().ToErrorMessageW());
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

        PathIterW fromIt { fromAbs };
        PathIterW toIt { toAbs };
        while (fromIt != PathIterW {} && toIt != PathIterW {} && *fromIt == *toIt)
        {
            ++fromIt;
            ++toIt;
        }

        if (fromIt == PathIterW {} && toIt == PathIterW {})
        {
            return L".";   // both paths are the same
        }

        Int32 upLevels = 0;
        for (const WStringView slice: fromIt)
        {
            if (slice.IsFile())
            {
                break;
            }

            if (!slice.IsEmpty() && slice != L"." && slice != L"..")
            {
                ++upLevels;
            }
            else if (slice == L"..")
            {
                --upLevels;   // ".." cancels out a previous level
            }
        }

        if (upLevels < 0)
        {
            return {};
        }

        WString result = {};
        result.Reserve(upLevels * 2 + toAbs.GetLength());   // reserve enough capacity for the result
        for (Int32 i = 0; i < upLevels; ++i)
        {
            result.Join(L"..", eJoin::Directory);
        }

        for (const WStringView slice: toIt)
        {
            result.Join(slice);
        }

        result.Simplify();
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

        const bool             bIsDir = _path.IsDirectory();
        const PathStackWString path   = _path;

        wchar_t     absolute[k_maxPathLength];
        const DWORD len = ::GetFullPathNameW(path.GetPtr(), NumericCast<DWORD>(k_maxPathLength), absolute, nullptr);
        if (len == 0)
        {
            return {};
        }

        absolute[len] = CharTraits<wchar_t>::k_nullChar;   // null-terminate the absolute path
        if (bIsDir)
        {
            absolute[len + 1] = CharTraits<wchar_t>::k_pathPreferredSeparator;   // append path separator for directory
        }

        WString result = absolute;
        result.Simplify();
        return result;
    }

    WString Current()
    {
        wchar_t     current[k_maxPathLength];
        const DWORD len = ::GetCurrentDirectoryW(NumericCast<DWORD>(k_maxPathLength), current);
        if (len == 0)
        {
            JAM_LOG_ERROR("Failed to get current directory: {}", CreateWindowsLastErrorMessage());
            return {};
        }

        current[len]     = CharTraits<wchar_t>::k_nullChar;                 // null-terminate the current directory path
        current[len + 1] = CharTraits<wchar_t>::k_pathPreferredSeparator;   // append path separator for directory

        WString result = current;
        result.Simplify();
        return result;
    }

    WString Executable()
    {
        wchar_t     executable[k_maxPathLength];
        const DWORD len = ::GetModuleFileNameW(NULL, executable, NumericCast<DWORD>(k_maxPathLength));
        if (len == 0)
        {
            JAM_LOG_ERROR("Failed to get executable path: {}", CreateWindowsLastErrorMessage());
            return {};
        }

        executable[len] = CharTraits<wchar_t>::k_nullChar;   // null-terminate the executable path

        WString result = executable;
        result.Simplify();
        return result;
    }

    WString GenerateUniquePath(
        const WStringView _basePath)
    {
        if (!Utf16::Exists(_basePath))
        {
            return _basePath;   // base path does not exist, treat as unique
        }

        const bool  bIsDir = _basePath.IsDirectory();
        WStringView dir = {}, base = {}, ext = {};
        _basePath.Decompose(&dir, nullptr, &base, &ext);

        WString result = {};
        result.Reserve(_basePath.GetLength() + 10);   // reserve enough capacity for the result
        for (Int32 counter = 1;; ++counter)
        {
            result = dir;
            result.AppendFormat(L"{} ({}){}", base, counter, ext);
            if (bIsDir)
            {
                result.Append(CharTraits<wchar_t>::k_pathPreferredSeparator);   // append path separator for directory
            }

            if (!Utf16::Exists(result))
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
        const StringView                    _dir,
        const eMakeDirectoryCollisionPolicy _policy)
    {
        PathStackWString path;
        path.Encode(_dir);
        return Utf16::MakeDirectory(path, _policy);
    }

    bool Exists(
        const StringView _path)
    {
        PathStackWString path;
        path.Encode(_path);
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
        PathStackWString path;
        path.Encode(_path);
        return Utf16::Remove(path);
    }

    Error Copy(
        const StringView       _srcPath,
        const StringView       _dstPath,
        const eCollisionPolicy _policy)
    {
        PathStackWString srcPath;
        srcPath.Encode(_srcPath);

        PathStackWString dstPath;
        dstPath.Encode(_dstPath);

        return Utf16::Copy(srcPath, dstPath, _policy);
    }

    Error Rename(
        const StringView       _srcPath,
        const StringView       _dstPath,
        const eCollisionPolicy _policy)
    {
        PathStackWString srcPath;
        srcPath.Encode(_srcPath);

        PathStackWString dstPath;
        dstPath.Encode(_dstPath);

        return Utf16::Rename(srcPath, dstPath, _policy);
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
        PathStackWString path;
        path.Encode(_path);
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
                JAM_LOG_ERROR("Failed to create directory iterator for path '{}': {}", _path, iter.GetError().ToErrorMessage());
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
        PathStackWString from;
        from.Encode(_from);

        PathStackWString to;
        to.Encode(_to);

        const WString rel = Utf16::Relative(from, to);
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

        PathStackWString path;
        path.Encode(_path);

        const WString abs = Utf16::Absolute(path);
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

    String GenerateUniquePath(
        const StringView _basePath)
    {
        PathStackWString basePath;
        basePath.Encode(_basePath);
        const WString uniquePath = Utf16::GenerateUniquePath(basePath);
        return String::Encoded(uniquePath.ToStringView());
    }

    bool ComparePathEqual(
        const StringView _path0,
        const StringView _path1)
    {
        PathStackWString path0;
        path0.Encode(_path0);

        PathStackWString path1;
        path1.Encode(_path1);

        return Utf16::ComparePathEqual(path0, path1);
    }

}   // namespace Utf8

}   // namespace Jam::FileSystem