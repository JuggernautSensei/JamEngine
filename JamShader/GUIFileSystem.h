#pragma once

// GUI Friendly Operation

namespace GUI
{

bool MakeDirectory(StringView _path, FileSystem::eMakeDirectoryCollisionPolicy _policy);
bool Copy(StringView _src, StringView _dst, FileSystem::eCollisionPolicy _policy);
bool Rename(StringView _src, StringView _dst, FileSystem::eCollisionPolicy _policy);
bool Remove(StringView _src);      // file or empty directory
bool RemoveAll(StringView _src);   // file or directory with all its content

}   // namespace GUI