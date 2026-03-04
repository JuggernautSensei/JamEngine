#pragma once

namespace Jam
{

class Texture;

struct FileEntry : public RefCounted
{
    [[nodiscard]] bool IsDirectory() const
    {
        return path.IsDirectory();
    }

    String       path       = {};
    String       name       = {};
    Ref<Texture> pThumbnail = nullptr;
};

struct DirEntry : public RefCounted
{
    [[nodiscard]] bool IsRoot() const
    {
        return pParent == nullptr;
    }

    String path    = {};
    String name    = {};
    bool   bOpened = false;   // for use gui

    Vector<Ref<DirEntry>> children = {};
    Ref<DirEntry>         pParent  = nullptr;
};

}   // namespace Jam