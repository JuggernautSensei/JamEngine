#pragma once

#include "EditorWidgets.h"
#include "HistoryStack.h"

#include <efsw/efsw.hpp>

#include "JsonReader.h"
#include "JsonWriter.h"
#include "ThumbnailCache.h"

namespace jam
{
enum class ePrimitiveCreationType;

class FileDeletedEvent;
class FileRenamedEvent;
class FileModifiedEvent;
class FileAddedEvent;
class Event;

class Editor_FileBrowser
{
    // only file directory structure
    struct DirNode
    {
        std::string     name       = {};
        fs::path        path       = {};
        Vector<DirNode> children   = {};
        bool            bIsFocused = false;
    };

    // all file entry (regular + directory)
    struct FileEntry
    {
        std::string       filename   = {};
        fs::path          path       = {};
        bool              bIsDir     = false;
        Ref<ImageTexture> pThumbnail = nullptr;
    };

public:
    void Init();
    void Shutdown() const;
    void OnEvent(Event& _event);
    void OnImGuiRender(bool& _pOpen);

    void Serialize(JsonWriter _writer);
    void Deserialize(JsonReader _reader);

private:
    // widgets
    void Widget_FileEntry_(const FileEntry& _entry, ImVec2 _size);
    void Widget_UndoButton_();
    void Widget_RedoButton_();
    void Widget_BackButton_();
    void Widget_FileEntries();
    void Widget_DirTree_();
    void Widget_DirNode_Recursive_(const DirNode& _node);

    // popup
    void Menu_FileEntriesPanel();
    void Menu_CreateFileEntry() const;
    void Menu_ImportAsset(const fs::path& _path) const;

    // file entries
    void GatherFileEntries_();

    // directory tree
    void                  BuildDirTree_();
    [[nodiscard]] DirNode BuildDirNode_Recursive_(const fs::path& _path);
    void                  MarkFocuedDirNode_();
    bool                  MarkFocusedDirNode_Recursive_(DirNode& _node);

    // navigation
    void SetFocusDir_(const fs::path& _dir, bool _bPushHistroyStack = true);
    void GoToParentDir_();

    // event handlers
    void OnFileAddEvent_(const FileAddedEvent& _event);
    void OnFileDeleteEvent_(const FileDeletedEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);

    // file operations
    void MoveFile_(const fs::path& _path, const fs::path& _newDir) const;
    void DeleteFile_(const fs::path& _path) const;
    void CreateFolder_(const fs::path& _parentDir) const;

    // dialog 
    void OnClickRemoveNotEmptyDirectoryYesButton_(const fs::path& _dir) const;

    // file explorer operations
    void OpenFileAsDefaultApplication_(const fs::path& _path) const;
    void OpenFileOnFileExplorer_(const fs::path& _path) const;

    // file watcher
    Scoped<efsw::FileWatcher>       m_pFileWatcher       = nullptr;
    Scoped<efsw::FileWatchListener> m_pFileWatchListener = nullptr;
    efsw::WatchID                   m_watchID            = 0;

    // navigation state
    fs::path               m_focusedDir          = {};
    bool                   m_bIsRootDirFocused = false;
    HistoryStack<fs::path> m_historyStack      = {};

    // directory tree
    DirNode m_rootDirNode = {};

    // misc
    ThumbnailCache m_thumbnailCache = {};

    // file entries
    Vector<FileEntry> m_fileEntries;
    ImGuiTextFilter   m_fileEntryFilter;
    float             m_fileEntryGatheringCounterSec  = 0.f;
    constexpr float   k_fileEntryGatheringDebounceSec = 0.5f;

    // update flags
    bool m_bNeedBuildDirTree      = true;
    bool m_bNeedGatherFileEntries = true;

    float                  m_zoom        = 1.f;
    constexpr static float k_minZoomPerc = 50.f;
    constexpr static float k_maxZoomPerc = 300.f;
};

}   // namespace jam
