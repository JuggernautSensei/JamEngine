#pragma once
#include "../JamEngine/FileWatcher.h"

class ProjectUnloadedEvent;
class ProjectChangedEvent;
class MainScene;

struct FileItem
{
    String path           = {};
    String filename       = {};
    bool   bOpened        = false;   // use for directory only
    bool   bOpenRequested = false;   // use for directory only

    Vector<Scoped<FileItem>> children = {};
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

    void                           GatherFileEntriesIfNeed_(float _dt);
    [[nodiscard]] Scoped<FileItem> GatherFileEntries_Recursive_(StringView _path, StringView _focusingPath, bool& _bFoundFocusingItem);

    void Widget_FileItem_Recursive_(FileItem* _pItem);
    void SetFocusingFileItem_(FileItem* _pItem);

    void OpenInFileExplorer_(StringView _path) const;
    void OpenAsDefaulApplication_(StringView _path) const;

    Scoped<FileWatcher>    m_pFileWatcher                         = nullptr;
    Scoped<FileItem>       m_rootItem                             = nullptr;
    FileItem*              m_pFocusingItem                        = nullptr;
    Project*               m_pProjectCache                        = nullptr;
    bool                   m_bFileEntriesDirty                    = true;
    float                  m_fileEntryGatheringIntervalCounterSec = 0.1f;
    constexpr static float k_fileEntryGatheringDebounceTimeSec    = 0.1f;
};
