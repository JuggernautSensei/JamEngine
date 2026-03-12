#include "pch.h"
#include "GUIFileSystem.h"

#include "ShaderApp.h"

namespace GUI
{

bool MakeDirectory(
    const StringView                                _path,
    const FileSystem::eMakeDirectoryCollisionPolicy _policy)
{
    if (const Error err = FileSystem::MakeDirectory(_path, _policy))
    {
        GetShaderApp().PushMessageDialog(
            "create directory failed",
            String::Formatted("failed to create directory \"{}\". {}.", _path, err.ToErrorMessage()));

        return false;
    }

    return true;
}

bool Copy(
    const StringView                   _src,
    const StringView                   _dst,
    const FileSystem::eCollisionPolicy _policy)
{
    if (const Error err = FileSystem::Copy(_src, _dst, _policy))
    {
        GetShaderApp().PushMessageDialog(
            "copy failed",
            String::Formatted("failed to copy from \"{}\" to \"{}\". {}.", _src, _dst, err.ToErrorMessage()));

        return false;
    }
    return true;
}

bool Rename(
    const StringView                   _src,
    const StringView                   _dst,
    const FileSystem::eCollisionPolicy _policy)
{
    if (const Error err = FileSystem::Rename(_src, _dst, _policy))
    {
        GetShaderApp().PushMessageDialog(
            "rename failed",
            String::Formatted("failed to rename from \"{}\" to \"{}\". {}.", _src, _dst, err.ToErrorMessage()));

        return false;
    }

    return true;
}

bool Remove(
    const StringView _src)
{
    if (_src.IsDirectory() && !FileSystem::IsEmpty(_src))
    {
        const auto OnConfirm = Bind(
            [path = String { _src }] {
                RemoveAll(path);
            });

        GetShaderApp().PushConfirmationDialog(
            "delete directory",
            String::Formatted("the directory \"{}\" is not empty. Do you want to delete it and all of its contents?", _src),
            OnConfirm);

        return false;
    }

    return RemoveAll(_src);
}

bool RemoveAll(
    const StringView _src)
{
    if (const Error err = FileSystem::Remove(_src))
    {
        GetShaderApp().PushMessageDialog(
            "delete failed",
            String::Formatted("failed to delete \"{}\" and all of its contents. {}.", _src, err.ToErrorMessage()));

        return false;
    }

    return true;
}

bool CheckExisting(
    const StringView _path)
{
    if (!FileSystem::Exists(_path))
    {
        GetShaderApp().PushMessageDialog(
            "file not found",
            String::Formatted("the file or directory \"{}\" does not exist.", _path));

        return false;
    }

    return true;
}

bool CheckNotExisting(
    const StringView _path)
{
    if (FileSystem::Exists(_path))
    {
        GetShaderApp().PushMessageDialog(
            "file already exists",
            String::Formatted("the file or directory \"{}\" already exists.", _path));

        return false;
    }

    return true;
}

}   // namespace GUI