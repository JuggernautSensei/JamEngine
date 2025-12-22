#pragma once

#include "EditorWidgets.h"
#include "Graphics.h"
#include "HistoryStack.h"

#include <efsw/efsw.hpp>

namespace jam
{

class FileDeletedEvent;
class FileRenamedEvent;
class FileModifiedEvent;
class FileAddedEvent;
class Event;

class Editor_FileBrowser
{
    class Thumbnail : public RefCounted
    {
    public:
        Thumbnail();
        ~Thumbnail() override;

        Thumbnail(const Thumbnail&)            = delete;
        Thumbnail& operator=(const Thumbnail&) = delete;

        Thumbnail(Thumbnail&& other) noexcept;
        Thumbnail& operator=(Thumbnail&& other) noexcept;

        bool Load(const fs::path& _path);
        void LoadDummy();

        [[nodiscard]] bool            IsDummy() const;
        [[nodiscard]] ImTextureRef    GetImTexturRef() const;
        [[nodiscard]] Vec2i      GetResolution() const;
        [[nodiscard]] const fs::path& GetPath() const;

    private:
        void CleanUp_();

        fs::path      m_path   = {};
        Vec2i    m_res    = {};
        TextureHandle m_handle = k_invalidHandle;

        constexpr static SIZE k_thumbnailSize = { 128, 128 };
    };

    struct DirNode
    {
        std::string          name       = {};
        fs::path             path       = {};
        Vector<DirNode> children   = {};
        bool                 bIsFocused = false;
    };

    struct FileEntry
    {
        std::string    name;
        fs::path       path;
        bool           bIsDir     = false;
        Ref<Thumbnail> pThumbnail = nullptr;
    };

    class ThumbnailCache
    {
        struct Item
        {
            Int32  bufferIdx;
            UInt64 timePoint;
        };

    public:
        void                         Init();
        [[nodiscard]] Ref<Thumbnail> LoadThumbnail(const fs::path& _path);

    private:
        void                                CollectThumbnailsInDirectory_(const fs::path& _dirPath);
        [[nodiscard]] Optional<UInt64> GetLastModifiedTime_(const fs::path& _path) const;

        Ref<Thumbnail>              m_pDummyThumbnail = nullptr;
        HashMap<fs::path, Item>     m_pathToItem      = {};
        Vector<Ref<Thumbnail>> m_thumbnails      = {};
        Int32                       m_end             = 0;
        Int32                       m_cap             = 0;
    };

public:
    void Init();
    void Shutdown() const;
    void OnEvent(Event& _event);
    void OnImGuiRender(bool& _pOpen);

    [[nodiscard]] Json OnSerialize() const;
    void               OnDeserialize(const Json& _json);

private:
    // widgets
    void Widget_FileEntry_(const FileEntry& _entry, ImVec2 _size);
    void Widget_DirNode_Recursive_(const DirNode& _node);
    void Widget_UndoButton_();
    void Widget_RedoButton_();
    void Widget_BackButton_();

    // popup
    void DrawFileEntriesChildPopup_();
    void DrawCreatePopup_() const;
    void DrawImportPopup_() const;
    void DrawImportFromFileEntryPopup_(const fs::path& _path) const;

    // file entries
    void DrawFileEntries_();
    void BuildFileEntries_();

    // directory tree
    void                  BuildDirTree_();
    [[nodiscard]] DirNode BuildDirNode_Recursive_(const fs::path& _path);
    void                  UpdateDirTree_();
    void                  UpdateDirNode_Recursive_(DirNode& _node);

    // navigation
    void SetFocusDir_(const fs::path& _dir, bool _bPushHistory = true);
    void GoToParentDir_();

    // drag and drop
    void OnDragDropTargetBackButton_() const;

    // event handlers
    void OnFileAddEvent_(const FileAddedEvent& _event);
    void OnFileDeleteEvent_(const FileDeletedEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);

    // file operations
    void RenameFile_(const fs::path& _src, const fs::path& _dst) const;
    void DeleteFile_(const fs::path& _path) const;
    void CreateNewFolder_(const fs::path& _parentDir) const;

    // file explorer operations
    void OpenFileAsDefaultApplication_(const fs::path& _path) const;
    void OpenFileOnFileExplorer_(const fs::path& _path) const;

    // file watcher
    std::unique_ptr<efsw::FileWatcher>       m_pFileWatcher       = nullptr;
    std::unique_ptr<efsw::FileWatchListener> m_pFileWatchListener = nullptr;
    efsw::WatchID                            m_watchID            = 0;

    // navigation state
    fs::path               m_focusDir          = {};
    bool                   m_bIsRootDirFocused = false;
    HistoryStack<fs::path> m_historyStack      = {};

    // directory tree
    DirNode m_rootDirNode = {};

    // file entries
    Vector<FileEntry> m_fileEntries;
    ImGuiTextFilter        m_fileEntryFilter;
    ThumbnailCache         m_thumbnailCache;

    // update flags
    bool m_bNeedUpdateDirTree     = true;
    bool m_bNeedUpdateFileEntries = true;

    float                  m_zoom        = 1.f;
    constexpr static float k_minZoomPerc = 50.f;
    constexpr static float k_maxZoomPerc = 300.f;
};

}   // namespace jam
