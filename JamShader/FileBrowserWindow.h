#pragma once
#include "../JamEngine/FileWatcher.h"

class ProjectUnloadedEvent;
class ProjectChangedEvent;
class MainScene;

struct FileEntry : public RefCounted
{
    String path    = {};
    String name    = {};
    bool   bOpened = false;   // use for directory only

    Vector<Ref<FileEntry>> children = {};
};

class FileBrowserWindow
{

public:
    void OnUpdate(float _dt);
    void OnImGuiRender();
    void OnEvent(Event& _event);

private:
    void InitFileWatcher_(StringView _path);

    void OnProjectChangedEvent_(const ProjectChangedEvent& _event);
    void OnFileAddedEvent_(const FileAddedEvent& _event);
    void OnFileDeletedEvent_(const FileDeletedEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);
    void OnFileWatcher_(const FileWatcherEvent& _event) const;

    void NewHLSLFile_(StringView _path) const;

    void                         GatherFileEntriesIfNeed_();
    [[nodiscard]] Ref<FileEntry> GatherFileEntries_Recursive_(StringView _path, StringView _focusingPath, bool& _bFoundFocusingEntry);

    void Menu_CreateFileEntry_(StringView _dir) const;
    void Widget_FileEntry_Recursive_(const Ref<FileEntry>& _pEntry);

    Scoped<FileWatcher> m_pFileWatcher      = nullptr;
    Ref<FileEntry>      m_rootEntry         = nullptr;
    bool                m_bFileEntriesDirty = true;
};
