#pragma once

#include "EditorWidgets.h"
#include "HistoryStack.h"
#include "ThumbnailCache.h"

#include <efsw/efsw.hpp>

namespace Jam
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
        Path            path        = {};
        StringView      name        = {};
        Vector<DirNode> children    = {};
        bool            bIsFocusing = false;
    };

    // all file entry (regular + directory)
    struct FileEntry
    {
        Path         path       = {};
        StringView   name       = {};
        bool         bIsDir     = false;
        Ref<Texture> pThumbnail = nullptr;
    };

public:
    Editor_FileBrowser();
    ~Editor_FileBrowser();

    Editor_FileBrowser(const Editor_FileBrowser&)                = delete;
    Editor_FileBrowser& operator=(const Editor_FileBrowser&)     = delete;
    Editor_FileBrowser(Editor_FileBrowser&&) noexcept            = delete;   // not impled yet.
    Editor_FileBrowser& operator=(Editor_FileBrowser&&) noexcept = delete;   // not impled yet.

    void OnEvent(Event& _event);
    void OnImGuiRender(bool& _pOpen);

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

private:
    // widgets
    void Widget_FileEntry_(const FileEntry& _entry, ImVec2 _size);
    void Widget_UndoButton_();
    void Widget_RedoButton_();
    void Widget_BackButton_();
    void Widget_FileEntries();
    void Widget_DirNode_Recursive_(const DirNode& _node);

    // popup
    void Menu_FileEntriesPanel();
    void Menu_CreateFileEntry() const;
    void Menu_ImportAsset(const Path& _path) const;

    // file entries
    void GatherFileEntries_();

    // directory tree
    void                  BuildDirTree_();
    [[nodiscard]] DirNode BuildDirNode_Recursive_(const Path& _path);
    bool                  MarkFocusingDirNode_Recursive_(DirNode& _node);

    // navigation
    void SetFocusingDir_(const Path& _dir, bool _bPushHistroyStack = true);
    void GoToParentDir_();

    // event handlers
    void OnFileAddEvent_(const FileAddedEvent& _event);
    void OnFileDeleteEvent_(const FileDeletedEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);
    void OnFileEvent_(const Path& _path);

    // shell helper
    void OpenFileInExplorer_(const Path& _path, bool _bSelected) const;
    void OpenFileWithDefaultApp_(const Path& _path) const;

    // file watcher
    Scoped<efsw::FileWatcher>       m_pFileWatcher       = nullptr;
    Scoped<efsw::FileWatchListener> m_pFileWatchListener = nullptr;
    efsw::WatchID                   m_watchID            = 0;

    // navigation state
    Path               m_focusingDir  = {};
    HistoryStack<Path> m_historyStack = {};

    // directory tree
    DirNode m_rootDirNode = {};

    // misc
    ThumbnailCache m_thumbnailCache = {};

    // file entries
    Vector<FileEntry>      m_fileEntries;
    ImGuiTextFilter        m_fileEntryFilter;
    float                  m_fileEntryGatheringCounterSec  = 0.f;
    constexpr static float k_fileEntryGatheringDebounceSec = 0.5f;

    // update flags
    bool m_bBuildDirTreeRequired      = true;
    bool m_bGatherFileEntriesRequried = true;

    float                  m_zoom        = 1.f;
    constexpr static float k_minZoomPerc = 50.f;
    constexpr static float k_maxZoomPerc = 300.f;

    constexpr static ImVec2 k_fileEntrySize     = { 100.f, 100.f };
    constexpr static float  k_fileEntrySpacingX = 20.f;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Editor_FileBrowser);

}   // namespace Jam
