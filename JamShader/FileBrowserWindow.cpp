#include "pch.h"
#include "FileBrowserWindow.h"

#include "Events.h"
#include "GUIFileSystem.h"
#include "ShaderApp.h"
#include "../JamEngine/DirIter.h"
#include "../JamEngine/EditorEvents.h"
#include "../JamEngine/EditorUtilities.h"
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
    Project* pProject = _event.GetProjectOrNull();
    if (pProject)
    {
        InitFileWatcher_(pProject->GetWorkingDirectory());
    }
    else
    {
        m_pFileWatcher.Reset();
    }
    m_pProject = pProject;
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
        Widget_FileItem_Recursive_(m_rootItem.GetPtr());
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
                [path = String(_event.path)]
                {
                    FileAddedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Deleted:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [path = String(_event.oldPath)]
                {
                    FileDeletedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Renamed:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [oldPath = String(_event.oldPath), newPath = String(_event.path)]
                {
                    FileRenamedEvent event { oldPath, newPath };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEventType::Modified:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [path = String(_event.path)]
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

    Expect<FileWriter> writer = FileWriter::Open(path, eFileWriterCollisionPolicy::Rename);
    if (!writer)
    {
        GetShaderApp().PushMessageDialog(
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

    const StringView workingDir   = m_pProject->GetWorkingDirectory();
    const StringView focusingPath = m_pFocusingItem ? m_pFocusingItem->path.ToStringView() : workingDir;
    bool             bFound       = false;
    m_rootItem                    = GatherFileEntries_Recursive_(workingDir, focusingPath, bFound);
    m_bFileEntriesDirty           = false;
}

Scoped<FileItem> FileBrowserWindow::GatherFileEntries_Recursive_(
    const StringView _path,
    const StringView _focusingPath,
    bool&            _bFoundFocusingItem)
{
    Scoped<FileItem> pItem = MakeScoped<FileItem>();

    if (_path.IsDirectory())
    {
        pItem->path     = _path;
        pItem->filename = pItem->path.GetFileName();

        const Expect<DirIter> iter = DirIter::Create(_path);
        JAM_ASSERT(iter, "Failed to create directory iterator for '{}': {}", _path, iter.GetError().ToErrorMessage());
        for (const StringView child: iter.GetValue())
        {
            pItem->children.emplace_back(GatherFileEntries_Recursive_(child, _focusingPath, _bFoundFocusingItem));
        }
    }
    else
    {
        pItem->path     = _path;
        pItem->filename = pItem->path.GetFileName();
    }

    if (!_bFoundFocusingItem && pItem->path == _focusingPath)
    {
        SetFocusingFileItem_(pItem.GetPtr());
        _bFoundFocusingItem = true;
    }

    return pItem;
}

void FileBrowserWindow::Widget_FileItem_Recursive_(
    FileItem* _pItem)
{
    const bool  bSelected = _pItem == m_pFocusingItem;
    const bool  bIsRoot   = _pItem == m_rootItem.GetPtr();
    const bool  bHasChild = !_pItem->children.empty();
    const char* label     = _pItem->filename.GetPtr();

    ImGui::PushID(_pItem->path.GetPtr());

    // tree node
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                 | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesToNodes;
        if (bSelected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (_pItem->path.IsFile() || !bHasChild)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        _pItem->bOpened = ImGui::TreeNodeEx("", flags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            SetFocusingFileItem_(_pItem);
        }
        ImGui::OpenPopupOnItemClick("Menu", 0);
    }

    // drag drop source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::TextUnformatted(label);
        SetPathDragDropPayload(_pItem->path);
        ImGui::EndDragDropSource();
    }

    // icon
    {
        const ImVec2        iconSize = { ImGui::GetFontSize(), ImGui::GetFontSize() };
        const Ref<Texture>& pIcon    = _pItem->path.IsFile() ? GetShaderApp().GetIcon(eIcon::RegularFile)
                                     : _pItem->bOpened       ? GetShaderApp().GetIcon(eIcon::OpenedDirectory)
                                                             : GetShaderApp().GetIcon(eIcon::ClosedDirectory);
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

        if (_pItem->path.IsDirectory())
        {
            ImGui::Separator();
            Menu_CreateFileItem_(_pItem->path);
        }

        if (!bIsRoot)
        {
            ImGui::Separator();
            if (ImGui::MenuItem("clone"))
            {
                GUI::FS::Copy(_pItem->path, _pItem->path, FileSystem::eCollisionPolicy::Rename);
            }

            if (ImGui::MenuItem("delete"))
            {
                GUI::FS::Remove(_pItem->path);
            }
        }

        ImGui::EndPopup();
    }

    // render children
    if (_pItem->bOpened)
    {
        for (Scoped<FileItem>& pChild: _pItem->children)
        {
            Widget_FileItem_Recursive_(pChild.GetPtr());
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FileBrowserWindow::SetFocusingFileItem_(
    FileItem* _pItem)
{
    if (_pItem == m_pFocusingItem)
    {
        return;
    }

    m_pFocusingItem = _pItem;
    FocusingFileItemChangedEvent event { m_pFocusingItem };
    GetApp().DispatchEvent(event);
}

void FileBrowserWindow::Menu_CreateFileItem_(
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