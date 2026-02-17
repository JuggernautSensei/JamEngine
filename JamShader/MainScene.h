#pragma once
#include <efsw/efsw.hpp>

#include "../JamEngine/DialogStack.h"
using FileWatcher       = efsw::FileWatcher;
using FileWatchListener = efsw::FileWatchListener;
using FileWatcherID     = efsw::WatchID;

class MainScene : public Scene
{
    JAM_SCENE_BODY(MainScene);

    struct FileEntry
    {
        List<FileEntry> children = {};
        Path            path     = {};
        StringView      name     = {};
        bool            bIsDir   = false;
    };
    using FileEntryIter = List<FileEntry>::iterator;

public:
    MainScene();

    void OnEnter() override;
    void OnRender(const RenderView& _desc) override;
    void OnImGuiRender() override;
    void OnEvent(Event& _event) override;

private:
    void OnFileAddedEvent_(const FileAddedEvent& _event);
    void OnFileDeletedEvent_(const FileDeletedEvent& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);

    void CreateNewDirectory_(const Path& _dir, StringView _nameHintOrEmpty);
    void CreateNewHLSLFile_(const Path& _dir, StringView _nameHintOrEmpty);
    void RemoveFile_(const Path& _path);
    void RemoveNonEmptyDirectory_(const Path& _dir);

    void                    BuildFileEntriesIfNeed_();
    [[nodiscard]] FileEntry BuildFileEntry_Recursive_(const Path& _path);
    void                    Panel_FileBrowser_();

    void Widget_FileEntry_Recursive_(const FileEntry& _entry);
    void Menu_CreateFileEntry_(const Path& _dir, StringView _nameHintOrEmpty);

    Scoped<FileWatcher>       m_pFileWatcher         = nullptr;
    Scoped<FileWatchListener> m_pFileWatcherListener = nullptr;
    FileWatcherID             m_fileWatcherID        = {};

    FileEntry                 m_rootFileEntry = {};
    HashMap<Path, FileEntry*> m_fileEntryLut  = {};
    Path                      m_focusingPath  = {};

    Path m_resourcePath      = {};
    bool m_bFileEntriesDirty = true;

    DialogStack m_dialogStack = {};

    constexpr static const char* k_resourceDirName = "Shaders";
};
