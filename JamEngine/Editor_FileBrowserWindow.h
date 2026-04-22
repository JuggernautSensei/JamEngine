#pragma once
#include "ImGuiEngine.h"
#include "EditorWidgets.h"
#include "FileEntry.h"
#include "FileWatcher.h"
#include "HistoryStack.h"
#include "ThumbnailCache.h"

namespace Jam
{
class FocusingDirEntryChangedEvent;

enum class ePrimitiveCreation;
class FileDeletedEvent;
class FileRenamedEvent;
class FileModifiedEvent;
class FileAddedEvent;
class Event;

class Editor_FileBrowserWindow
{
public:
    Editor_FileBrowserWindow();

    void OnUpdate(float _dt);
    void OnEvent(Event& _event);
    void OnImGuiRender(bool& _pOpen);

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

private:
    // widgets
    void Widget_FileEntry_(const Ref<FileEntry>& _pEntry, ImVec2 _size);
    void Widget_UndoButton_();
    void Widget_RedoButton_();
    void Widget_BackButton_();
    void Widget_FileEntries();
    void Widget_DirEntries();
    void Widget_DirEntries_Recursive_(const Ref<DirEntry>& _pEntry);

    // popup
    void Menu_FileEntriesPanel_();
    void Menu_CreateFileEntry() const;
    void Menu_ImportAsset(StringView _path) const;

    // entries
    void           GatherFileEntries_();
    Ref<FileEntry> GatherFileEntry_(StringView _path);

    void          GatherFilteredFileEntriesIfNeed_();
    void          GatherDirEntries_();
    Ref<DirEntry> GatherDirEntries_Recursive_(StringView _dir);

    // navigation
    void SetFocusingDirEntry_(const Ref<DirEntry>& _pEntry, bool _bPushHistroyStack);
    void GoToParentDir_();

    // event handlers
    void OnFileAddEvent_(const FileAddedEvent& _event);
    void OnFileDeleteEvent_(const FileDeletedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);
    void OnFileWatcherEvent_(const FileWatcherEvent& _event) const;
    void OnFocusingDirEntryChanged_(const FocusingDirEntryChangedEvent& _event);

    // shell helper
    void OpenInFileExplorer_(StringView _path, bool _bSelected) const;
    void OpenAsDefaultApp_(StringView _path) const;

    // file watcher
    Scoped<FileWatcher>         m_pFileWatcher = nullptr;
    HistoryStack<Ref<DirEntry>> m_historyStack = {};

    // dir entry
    Ref<DirEntry>                m_pRootDirNode   = nullptr;   // root dir entry. for display.
    StringHashMap<Ref<DirEntry>> m_dirEntryLookUp = {};        // [path, entry]

    Vector<Ref<FileEntry>>        m_filteredFileEntries = {};   // filtered entry. for display.
    StringHashMap<Ref<FileEntry>> m_fileEntryLookUp     = {};   // [path, entry]

    // misc
    ThumbnailCache m_thumbnailCache = {};

    // file entries
    ImGuiTextFilter        m_fileEntryFilter                  = {};
    float                  m_fileEntryFilterCounterSec        = 0.f;
    constexpr static float kFileEntryFilterUpdateIntervalSec = 0.5f;
    bool                   m_bGatherFilteredFileEntries       = true;

    float                  m_zoom        = 1.f;
    constexpr static float kMinZoomPerc = 50.f;
    constexpr static float kMaxZoomPerc = 300.f;

    constexpr static ImVec2 kFileEntrySize     = { 100.f, 100.f };
    constexpr static float  kFileEntrySpacingX = 20.f;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Editor_FileBrowserWindow);

}   // namespace Jam
