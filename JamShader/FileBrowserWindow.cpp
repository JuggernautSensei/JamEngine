#include "pch.h"
#include "FileBrowserWindow.h"

#include "Events.h"
#include "GUIFileSystem.h"
#include "MainScene.h"
#include "../JamEngine/DirIter.h"
#include "../JamEngine/EditorWidgets.h"

void FileBrowserWindow::InitFileWatcher_(
    const StringView _path)
{
    FileWatcherInfo info                     = {};
    info.path                                = _path;
    info.bRecursive                          = true;
    info.pollingDelayMs                      = 500;
    info.FileWatcherListener                 = Bind<&FileBrowserWindow::OnFileWatcher_>(this);
    Expect<Scoped<FileWatcher>> pFileWatcher = FileWatcher::Create(info);
    JAM_ASSERT(pFileWatcher, "Failed to create file watcher for '{}': {}", info.path, pFileWatcher.GetError().ToErrorMessage());
    m_pFileWatcher = pFileWatcher.Take();
}

void FileBrowserWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent& _event)
{
    const Ref<Project>& pProject = _event.GetProjectOrNull();
    if (pProject)
    {
        InitFileWatcher_(pProject->GetWorkingDirectory());
    }
    else
    {
        m_pFileWatcher.Reset();
    }
}

void FileBrowserWindow::OnUpdate(
    const float)
{
    GatherFileEntriesIfNeed_();
}

void FileBrowserWindow::OnImGuiRender()
{
    if (!ImGui::Begin("file browser"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginChild("FileEntriesPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
    {
        Widget_FileEntry_Recursive_(m_rootEntry);
    }
    ImGui::EndChild();

    ImGui::End();
}

void FileBrowserWindow::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileAddedEvent>(this, &FileBrowserWindow::OnFileAddedEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &FileBrowserWindow::OnFileDeletedEvent_);
    dispatcher.Dispatch<FileModifiedEvent>(this, &FileBrowserWindow::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileRenamedEvent>(this, &FileBrowserWindow::OnFileRenamedEvent_);
    dispatcher.Dispatch<ProjectChangedEvent>(this, &FileBrowserWindow::OnProjectChangedEvent_);
}

void FileBrowserWindow::OnFileAddedEvent_(
    const FileAddedEvent&)
{
    m_bFileEntriesDirty = true;
}

void FileBrowserWindow::OnFileDeletedEvent_(
    const FileDeletedEvent&)
{
    m_bFileEntriesDirty = true;
}

void FileBrowserWindow::OnFileModifiedEvent_(
    const FileModifiedEvent&)
{
    m_bFileEntriesDirty = true;
}

void FileBrowserWindow::OnFileRenamedEvent_(
    const FileRenamedEvent&)
{
    m_bFileEntriesDirty = true;
}

void FileBrowserWindow::OnFileWatcher_(
    const FileWatcherEvent& _event) const
{
    switch (_event.type)
    {
        case eFileEventType::Added:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [path = _event.newPath]
                {
                    FileAddedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Deleted:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [path = _event.oldPath]
                {
                    FileDeletedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Renamed:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [oldPath = _event.oldPath, newPath = _event.newPath]
                {
                    FileRenamedEvent event { oldPath, newPath };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Modified:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [path = _event.newPath]
                {
                    FileModifiedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        default:
            JAM_UNREACHABLE;
    }
}

void FileBrowserWindow::NewHLSLFile_(
    const StringView _path) const
{
    const String path = String::Joined(_path, "new shader.hlsl");

    Expect<FileWriter> writer = FileWriter::Open(_path);
    if (!writer)
    {
        GetMainScene().PushMessageDialog(
            "Failed to create file",
            String::Formatted("Failed to create file '{}': {}", path, writer.GetError().ToErrorMessage()));
    }
}

void FileBrowserWindow::GatherFileEntriesIfNeed_()
{
    if (!m_bFileEntriesDirty)
    {
        return;
    }

    const Ref<Project>&   pProject         = GetMainScene().GetProject();
    const StringView      workingDirectory = pProject->GetWorkingDirectory();
    const Ref<FileEntry>& pFocusedEntry    = GetMainScene().GetFocusingFileEntryOrNull();
    const StringView      focusingPath     = pFocusedEntry ? pFocusedEntry->path.ToStringView() : workingDirectory;

    bool bFoundFocusingEntry = false;
    m_rootEntry              = GatherFileEntries_Recursive_(workingDirectory, focusingPath, bFoundFocusingEntry);
    m_bFileEntriesDirty      = false;
}

Ref<FileEntry> FileBrowserWindow::GatherFileEntries_Recursive_(
    const StringView _path,
    const StringView _focusingPath,
    bool&            _bFoundFocusingEntry)
{
    Ref<FileEntry> entry = MakeRef<FileEntry>();

    if (_path.IsDirectory())
    {
        entry->path = _path;
        entry->name = entry->path.GetFileName();

        const Expect<DirIter> iter = DirIter::Create(_path);
        JAM_ASSERT(iter, "Failed to create directory iterator for '{}': {}", _path, iter.GetError().ToErrorMessage());
        for (const StringView child: iter.GetValue())
        {
            entry->children.emplace_back(GatherFileEntries_Recursive_(child, _focusingPath, _bFoundFocusingEntry));
        }
    }
    else
    {
        entry->path = _path;
        entry->name = entry->path.GetFileName();
    }

    if (!_bFoundFocusingEntry && entry->path == _focusingPath)
    {
        GetMainScene().SetFocusingFileEntry(entry);
        _bFoundFocusingEntry = true;
    }

    return entry;
}

void FileBrowserWindow::Widget_FileEntry_Recursive_(
    const Ref<FileEntry>& _pEntry)
{
    const bool  bSelected = _pEntry == GetMainScene().GetFocusingFileEntryOrNull();
    const bool  bIsRoot   = _pEntry == m_rootEntry;
    const bool  bHasChild = !_pEntry->children.empty();
    const char* label     = _pEntry->name.GetPtr();

    ImGui::PushID(_pEntry->path.GetPtr());

    // tree node
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                 | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesToNodes;
        if (bSelected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (_pEntry->path.IsFile() || !bHasChild)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        _pEntry->bOpened = ImGui::TreeNodeEx("", flags);
        if (ImGui::IsItemClicked())
        {
            GetMainScene().SetFocusingFileEntry(_pEntry);
        }
        ImGui::OpenPopupOnItemClick("Menu", 0);
    }

    // icon
    {
        const ImVec2        iconSize = { ImGui::GetFontSize(), ImGui::GetFontSize() };
        const Ref<Texture>& pIcon    = _pEntry->path.IsFile() ? GetMainScene().GetIcon(eIcon::RegularFile)
                                     : _pEntry->bOpened       ? GetMainScene().GetIcon(eIcon::OpenedDirectory)
                                                              : GetMainScene().GetIcon(eIcon::ClosedDirectory);
        ImGui::SameLine();
        Widget_Image(pIcon, iconSize);
    }

    // text
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }

    // tree node menu
    if (ImGui::BeginPopup("Menu"))
    {
        ImGui::Text(label);

        if (_pEntry->path.IsDirectory())
        {
            ImGui::Separator();
            Menu_CreateFileEntry_(_pEntry->path);
        }

        if (!bIsRoot)
        {
            ImGui::Separator();
            if (ImGui::MenuItem("clone"))
            {
                GUI::FS::Copy(_pEntry->path, _pEntry->path, FileSystem::eCollisionPolicy::Rename);
            }

            if (ImGui::MenuItem("delete"))
            {
                GUI::FS::Remove(_pEntry->path);
            }
        }

        ImGui::EndPopup();
    }

    // render children
    if (_pEntry->bOpened)
    {
        for (const Ref<FileEntry>& pChild: _pEntry->children)
        {
            Widget_FileEntry_Recursive_(pChild.GetPtr());
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FileBrowserWindow::Menu_CreateFileEntry_(
    const StringView _dir) const
{
    JAM_ASSERT(_dir.IsDirectory(), "String must be a directory");

    if (ImGui::MenuItem("create new hlsl file"))
    {
        NewHLSLFile_(_dir);
    }

    if (ImGui::MenuItem("create new directory"))
    {
        const String path = String::Joined(_dir, "new directory/");
        GUI::FS::MakeDirectory(path, FileSystem::eMakeDirectoryCollisionPolicy::Rename);
    }
}