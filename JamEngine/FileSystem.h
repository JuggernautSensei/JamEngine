#pragma once

namespace Jam::FileSystem
{

enum eFileType
{
    Unknown,
    File,
    Directory,
    SymbolicLink
};

struct FileAttribute
{
    eFileType type           = eFileType::Unknown;
    UInt64    creationTime   = 0;
    UInt64    lastAccessTime = 0;
    UInt64    lastWriteTime  = 0;
    UInt64    fileSize       = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class eCollisionPolicy
{
    Fail,
    Skip,
    Overwrite,
    Rename,   // e.g. "file.txt" -> "file (1).txt"
};

enum class eMakeDirectoryCollisionPolicy
{
    Fail,
    Skip,
    Rename
};

namespace Utf16   // windows native
{
    Error                       MakeDirectory(WStringView _dir, eMakeDirectoryCollisionPolicy _policy = eMakeDirectoryCollisionPolicy::Skip);
    [[nodiscard]] bool          Exists(WStringView _path);
    [[nodiscard]] bool          IsDirectory(WStringView _path);
    [[nodiscard]] bool          IsFile(WStringView _path);
    [[nodiscard]] bool          IsSymbolicLink(WStringView _path);
    Error                       Remove(WStringView _path);   // always remove recusive. if you want to check the directory is empty, use IsEmpty() before calling OnRemove()
    Error                       Copy(WStringView _srcPath, WStringView _dstPath, eCollisionPolicy _policy = eCollisionPolicy::Fail);
    Error                       Rename(WStringView _srcPath, WStringView _dstPath, eCollisionPolicy _policy = eCollisionPolicy::Fail);
    [[nodiscard]] UInt64        QueryFileSize(WStringView _path);
    [[nodiscard]] UInt64        QueryLastWriteTime(WStringView _path);
    [[nodiscard]] FileAttribute QueryFileAttribute(WStringView _path);
    [[nodiscard]] bool          IsEmpty(WStringView _path);
    [[nodiscard]] WString       Relative(WStringView _from, WStringView _to);
    [[nodiscard]] WString       Absolute(WStringView _path);
    [[nodiscard]] WString       Current();
    [[nodiscard]] WString       Executable();
    [[nodiscard]] WString       GenerateUniquePath(WStringView _basePath);   // aaa/bbb/ccc.txt -> aaa/bbb/ccc (1).txt -> aaa/bbb/ccc (2).txt -> ...
    [[nodiscard]] bool          ComparePathEqual(WStringView _path0, WStringView _path1);   

}   // namespace Utf16

inline namespace Utf8   // jam engine native
{
    Error                       MakeDirectory(StringView _dir, eMakeDirectoryCollisionPolicy _policy = eMakeDirectoryCollisionPolicy::Skip);
    [[nodiscard]] bool          Exists(StringView _path);
    [[nodiscard]] bool          IsDirectory(StringView _path);
    [[nodiscard]] bool          IsFile(StringView _path);
    [[nodiscard]] bool          IsSymbolicLink(StringView _path);
    Error                       Remove(StringView _path);   // always remove recusive. if you want to check the directory is empty, use IsEmpty() before calling OnRemove()
    Error                       Copy(StringView _srcPath, StringView _dstPath, eCollisionPolicy _policy = eCollisionPolicy::Fail);
    Error                       Rename(StringView _srcPath, StringView _dstPath, eCollisionPolicy _policy = eCollisionPolicy::Fail);
    [[nodiscard]] UInt64        QueryFileSize(StringView _path);
    [[nodiscard]] UInt64        QueryLastWriteTime(StringView _path);
    [[nodiscard]] FileAttribute QueryFileAttribute(StringView _path);
    [[nodiscard]] bool          IsEmpty(StringView _path);
    [[nodiscard]] String        Relative(StringView _from, StringView _to);
    [[nodiscard]] String        Absolute(StringView _path);
    [[nodiscard]] String        Current();
    [[nodiscard]] String        Executable();
    [[nodiscard]] String        GenerateUniquePath(StringView _basePath);   // aaa/bbb/ccc.txt -> aaa/bbb/ccc (1).txt -> aaa/bbb/ccc (2).txt -> ...
    [[nodiscard]] bool          ComparePathEqual(StringView _path0, StringView _path1);

}   // namespace Utf8

}   // namespace Jam::FileSystem