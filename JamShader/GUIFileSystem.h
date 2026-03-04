#pragma once

// GUI Friendly Operation

namespace GUI
{
namespace FS   // file system
{

    bool MakeDirectory(StringView _path, FileSystem::eMakeDirectoryCollisionPolicy _policy);
    bool Copy(StringView _src, StringView _dst, FileSystem::eCollisionPolicy _policy);
    bool Rename(StringView _src, StringView _dst, FileSystem::eCollisionPolicy _policy);
    bool Remove(StringView _src);      // file or empty directory
    bool RemoveAll(StringView _src);   // file or directory with all its content

    bool CheckExisting(StringView _path);
    bool CheckNotExisting(StringView _path);

}   // namespace FS
}   // namespace GUI