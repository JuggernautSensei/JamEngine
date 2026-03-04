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
        WStackString<k_maxPathLength> path;
        for (const WStringView subPath: WPathIter { _dir })
        {
            path.Join(subPath);
            if (!Exists(path))
            {
                if (::CreateDirectoryW(path.GetPtr(), NULL) == FALSE)
                {
                    JAM_LOG_ERROR(L"Failed to create directory '{}': {}", subPath, CreateWindowsLastErrorMessageW());
                    return ErrorFromPlatformErrorCode(::GetLastError());
                }
            }
        }

        return Error::None;
    }

    bool Exists(
        const WStringView _path)
    {
        WStackString<k_maxPathLength> path;
        path.Join(_path);
        return ::GetFileAttributesW(path.GetPtr()) != INVALID_FILE_ATTRIBUTES;
    }

    bool IsDirectory(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::Directory;
    }

    bool IsFile(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::File;
    }

    bool IsSymbolicLink(
        const WStringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::SymbolicLink;
    }

    Error Remove(
        const WStringView _path)
    {
        if (_path.IsDirectory())
        {
            // remove all entries in the directory
            Expect<WDirIter> iter = WDirIter::Create(_path);
            if (!iter)
            {
                return iter.GetError();   // failed to create directory iterator
            }

            for (const WStringView childPath: iter.GetValue())
            {
                if (const Error err = Remove(childPath))
                {
                    return err;   // failed to remove an entry in the directory
                }
            }

            // remove empty directory
            WStackString<k_maxPathLength> path;
            path.Join(_path);
            if (::RemoveDirectoryW(path.GetPtr()) == FALSE)
            {
                JAM_LOG_ERROR(L"Failed to remove directory '{}': {}", _path, CreateWindowsLastErrorMessageW());
                return ErrorFromPlatformErrorCode(::GetLastError());
            }
        }
        else
        {
            WStackString<k_maxPathLength> path;
            path.Join(_path);
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
            Expect<WDirIter> iter = WDirIter::Create(_srcPath);
            if (!iter)
            {
                return iter.GetError();   // failed to create directory iterator
            }

            for (const WStringView childSrcPath: iter.GetValue())
            {
                WStackString<k_maxPathLength> childDstPath;
                childDstPath.Join(_dstPath);
                childDstPath.Join(childSrcPath);

                if (const Error err = Copy(childSrcPath, childDstPath, _policy))
                {
                    return err;   // failed to copy an entry in the directory
                }
            }
        }
        else
        {
            WStackString<k_maxPathLength> srcPath;
            srcPath.Join(_srcPath);

            WStackString<k_maxPathLength> dstPath;
            dstPath.Join(_dstPath);

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

        WStackString<k_maxPathLength> srcPath;
        srcPath.Join(_srcPath);

        WStackString<k_maxPathLength> dstPath;
        dstPath.Join(_dstPath);

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
        WStackString<k_maxPathLength> path;
        path.Join(_path);

        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (::GetFileAttributesExW(path.GetPtr(), GetFileExInfoStandard, &data) == FALSE)
        {
            JAM_ASSERT(0, "Failed to query file attributes");
        }

        FileAttribute attrib = {};
        attrib.type          = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY     ? eFileType::Directory
                             : data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? eFileType::SymbolicLink
                                                                                    : eFileType::File;

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
            Expect<WDirIter> iter = WDirIter::Create(_path);
            if (!iter)
            {
                JAM_LOG_ERROR(L"Failed to create directory iterator for path '{}': {}", _path, iter.GetError().ToErrorMessageW());
                return false;   // treat as not empty if failed to create directory iterator
            }

            return iter.GetValue() == WDirIter {};
        }

        return QueryFileSize(_path) == 0;
    }

    WString Relative(
        const WStringView _from,
        const WStringView _to)
    {
        const WString from = Absolute(_from);
        const WString to   = Absolute(_to);

        WPathIter fromIt { from };
        WPathIter toIt { to };
        while (fromIt != WPathIter {} && toIt != WPathIter {} && *fromIt == *toIt)
        {
            ++fromIt;
            ++toIt;
        }

        if (fromIt == WPathIter {} && toIt == WPathIter {})
        {
            return L".";   // both paths are the same
        }

        Int32 upLevels = 0;
        for (const WStringView path: fromIt)
        {
            if (path.IsFile())
            {
                break;
            }

            if (!path.IsEmpty() && path != L"." && path != L"..")
            {
                ++upLevels;
            }
            else if (path == L"..")
            {
                --upLevels;   // ".." cancels out a previous level
            }
        }

        if (upLevels < 0)
        {
            return {};
        }

        WString result = {};
        for (Int32 i = 0; i < upLevels; ++i)
        {
            result.Join(L"..", eJoinType::Directory);
        }

        for (const WStringView path: toIt)
        {
            result.Join(path);
        }

        return result;
    }

    WString Absolute(
        const WStringView _path)
    {
        if (_path.IsAbsolute())
        {
            return _path;   // if already absolute, return as is
        }

        WStackString<k_maxPathLength> path;
        path.Join(_path);

        wchar_t     absolute[k_maxPathLength];
        const DWORD len = ::GetFullPathNameW(_path.GetPtr(), k_maxPathLength, absolute, nullptr);
        if (len == 0)
        {
            JAM_LOG_ERROR(L"Failed to get absolute path for '{}': {}", _path, CreateWindowsLastErrorMessageW());
            return {};
        }

        if (_path.IsDirectory())
        {
            absolute[len]     = CharTraits<wchar_t>::k_pathPreferredSeparator;
            absolute[len + 1] = CharTraits<wchar_t>::k_nullChar;
        }
        else
        {
            absolute[len] = CharTraits<wchar_t>::k_nullChar;
        }

        WString result = {};
        result.Join(absolute);
        result.Simplify();
        return result;
    }

    WString Current()
    {
        wchar_t     current[k_maxPathLength];
        const DWORD len = ::GetCurrentDirectoryW(k_maxPathLength, current);
        if (len == 0)
        {
            JAM_LOG_ERROR("Failed to get current directory: {}", CreateWindowsLastErrorMessage());
            return {};
        }

        current[len]     = CharTraits<wchar_t>::k_pathPreferredSeparator;
        current[len + 1] = CharTraits<wchar_t>::k_nullChar;

        WString result = {};
        result.Join(current);
        result.Simplify();
        return result;
    }

    WString Executable()
    {
        wchar_t     executable[k_maxPathLength];
        const DWORD len = ::GetModuleFileNameW(NULL, executable, k_maxPathLength);
        if (len == 0)
        {
            JAM_LOG_ERROR("Failed to get executable path: {}", CreateWindowsLastErrorMessage());
            return {};
        }

        executable[len] = CharTraits<wchar_t>::k_nullChar;

        WString result = {};
        result.Join(executable);
        result.Simplify();
        return result;
    }

    WString GenerateUniquePath(
        const WStringView _basePath)
    {
        const bool  bIsDir = _basePath.IsDirectory();
        WStringView dir = {}, base = {}, ext = {};
        _basePath.Decompose(&dir, nullptr, &base, &ext);

        WString result  = {};
        Int32   counter = 1;
        do
        {
            result.Set(dir);
            result.Append(base);
            result.AppendFormat(L" ({})", counter);
            result.Append(ext);
            if (bIsDir)
            {
                result.Append(L"/");
            }
            ++counter;
        } while (Utf16::Exists(result));

        return result;
    }

    bool ComparePathEqual(
        const WStringView _path0,
        const WStringView _path1)
    {
        return Absolute(_path0) == Absolute(_path1);
    }

}   // namespace Utf16

namespace Utf8
{

    Error MakeDirectory(
        const StringView                    _dir,
        const eMakeDirectoryCollisionPolicy _policy)
    {
        WStackString<k_maxPathLength> path;
        path.Encode(_dir);
        return Utf16::MakeDirectory(path, _policy);
    }

    bool Exists(
        const StringView _path)
    {
        WStackString<k_maxPathLength> path;
        path.Encode(_path);
        return Utf16::Exists(path);
    }

    bool IsDirectory(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::Directory;
    }

    bool IsFile(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::File;
    }

    bool IsSymbolicLink(
        const StringView _path)
    {
        return QueryFileAttribute(_path).type == eFileType::SymbolicLink;
    }

    Error Remove(
        const StringView _path)
    {
        WStackString<k_maxPathLength> path;
        path.Encode(_path);
        return Utf16::Remove(path);
    }

    Error Copy(
        const StringView       _srcPath,
        const StringView       _dstPath,
        const eCollisionPolicy _policy)
    {
        WStackString<k_maxPathLength> srcPath;
        srcPath.Encode(_srcPath);

        WStackString<k_maxPathLength> dstPath;
        dstPath.Encode(_dstPath);

        return Utf16::Copy(srcPath, dstPath, _policy);
    }

    Error Rename(
        const StringView       _srcPath,
        const StringView       _dstPath,
        const eCollisionPolicy _policy)
    {
        WStackString<k_maxPathLength> srcPath;
        srcPath.Encode(_srcPath);

        WStackString<k_maxPathLength> dstPath;
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
        WStackString<k_maxPathLength> path;
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
        WStackString<k_maxPathLength> from;
        from.Encode(_from);

        WStackString<k_maxPathLength> to;
        to.Encode(_to);

        const WString rel = Utf16::Relative(from, to);
        return String::Encoded(rel.ToStringView());
    }

    String Absolute(
        const StringView _path)
    {
        if (_path.IsAbsolute())
        {
            return _path;
        }

        WStackString<k_maxPathLength> path;
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
        WStackString<k_maxPathLength> basePath;
        basePath.Encode(_basePath);
        const WString uniquePath = Utf16::GenerateUniquePath(basePath);
        return String::Encoded(uniquePath.ToStringView());
    }

    bool ComparePathEqual(
        const StringView _path0,
        const StringView _path1)
    {
        WStackString<k_maxPathLength> path0;
        path0.Encode(_path0);
        WStackString<k_maxPathLength> path1;
        path1.Encode(_path1);
        return Utf16::ComparePathEqual(path0, path1);
    }

}   // namespace Utf8

}   // namespace Jam::FileSystem