#include "pch.h"

#include "Editor_FileBrowserWindow.h"

#include "Application.h"
#include "DirIter.h"
#include "MaterialCreationDialog.h"
#include "PrimitiveCreationDialog.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Events.h"
#include "FileSystem.h"
#include "ImageImportDialog.h"
#include "ModelImportDialog.h"
#include "RenameFileDialog.h"
#include "EditorEvents.h"

namespace Jam
{

Editor_FileBrowserWindow::Editor_FileBrowserWindow()
{
    const StringView contentsDir = GetApp().GetContentsDirectory();

    // file watcher
    FileWatcherInfo info     = {};
    info.path                = contentsDir;
    info.bRecursive          = true;
    info.pollingDelayMs      = 100;
    info.FileWatcherListener = Bind<&Editor_FileBrowserWindow::OnFileWatcherEvent_>(this);

    Expect<Scoped<FileWatcher>> pFileWatcher = FileWatcher::Create(info);
    JAM_ASSERT(pFileWatcher, "Failed to create file watcher: %s", pFileWatcher.GetError().ToErrorMessage());
    m_pFileWatcher = pFileWatcher.Take();

    // gather
    GatherDirEntries_();
    GatherFileEntries_();
}

void Editor_FileBrowserWindow::OnUpdate(
    const float _dt)
{
    m_fileEntryFilterCounterSec += _dt;
    GatherFilteredFileEntriesIfNeed_();
}

void Editor_FileBrowserWindow::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileAddedEvent>(this, &Editor_FileBrowserWindow::OnFileAddEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &Editor_FileBrowserWindow::OnFileDeleteEvent_);
    dispatcher.Dispatch<FileRenamedEvent>(this, &Editor_FileBrowserWindow::OnFileRenamedEvent_);
    dispatcher.Dispatch<FocusingDirEntryChangedEvent>(this, &Editor_FileBrowserWindow::OnFocusingDirEntryChanged_);

    m_thumbnailCache.OnEvent(_event);
}

void Editor_FileBrowserWindow::OnImGuiRender(
    bool& _pOpen)
{
    if (!ImGui::Begin("file browser", &_pOpen))
    {
        ImGui::End();
        return;
    }

    // toolbar
    {
        // create
        ImGui::Button("create");
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            Menu_CreateFileEntry();
            ImGui::EndPopup();
        }

        // import
        ImGui::SameLine();
        ImGui::Button("import");
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            const Ref<DirEntry>& pEntry = GetEditor().GetFocusingDirEntry();
            Menu_ImportAsset(pEntry->path);
            ImGui::EndPopup();
        }

        // undo
        ImGui::SameLine();
        Widget_UndoButton_();

        // redo
        ImGui::SameLine();
        Widget_RedoButton_();

        // back
        ImGui::SameLine();
        Widget_BackButton_();

        // current path
        ImGui::SameLine();
        SetNextItemToAvailWidth();
        Widget_ReadOnlyInputText("##path", GetEditor().GetFocusingDirEntry()->name);
    }
    ImGui::Separator();

    // file directory
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("directories");

        ImGui::SetNextWindowSizeConstraints({ 200.f, 0.f }, { 400.f, Limits<float>::kMax });
        if (ImGui::BeginChild("DirectoryTreePanel", ImVec2 { 0.f, 0.f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
        {
            Widget_DirEntries_Recursive_(m_pRootDirNode);
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    // file entries
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        // render filter input
        Widget_Text("filter:", true);

        // filter input
        ImGui::SameLine();
        if (m_fileEntryFilter.Draw("##Filter", 150.f))
        {
            m_bGatherFilteredFileEntries = true;
        }

        // filter clear
        ImGui::SameLine();
        if (ImGui::Button("clear"))
        {
            m_bGatherFilteredFileEntries = true;
        }

        // zoom control
        ImGui::SameLine();
        {
            ImGui::SetNextItemWidth(150.f);
            Widget_DragPercent("##Zoom", m_zoom, 0.5f, kMinZoomPerc, kMaxZoomPerc, "zoom: %.0f%%");
        }

        // file entries panel
        if (ImGui::BeginChild("FileEntriesPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
        {
            // draw file entries
            Widget_FileEntries();

            // menu
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                Menu_FileEntriesPanel_();
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::End();
}

void Editor_FileBrowserWindow::WriteToArchive(
    ArchiveWriter& _writer) const
{
    _writer.WriteField("zoom", m_zoom);
}

Error Editor_FileBrowserWindow::ReadFromArchive(
    ArchiveReader& _reader)
{
    return _reader.ReadField("zoom", m_zoom);
}

void Editor_FileBrowserWindow::Widget_FileEntry_(
    const Ref<FileEntry>& _pEntry,
    const ImVec2          _size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0);   // invisible button bg
    ImGui::BeginGroup();
    ImGui::PushID(_pEntry->name.GetPtr());

    // draw button
    const ImVec2 itemLeftTop = ImGui::GetCursorScreenPos();
    ImGui::Button("##fileEntry", _size);
    ImGui::OpenPopupOnItemClick("Menu");   // right click popup

    // button state
    const bool bTooltip        = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    const bool bDoubleClicked  = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    bool       bIsDragDropping = false;   // tooltip과 drag drop이 동시에 뜨는걸 방지하기 위한 플래그

    // drag drop source
    if (ImGui::BeginDragDropSource())
    {
        bIsDragDropping = true;

        // set payload
        SetPathDragDropPayload(_pEntry->path);

        // preview
        ImGui::TextUnformatted(_pEntry->name.GetPtr());
        ImGui::EndDragDropSource();
    }

    // drag drop target (디렉토리인 경우만. accept path의 parent dir을 자신으로 변경)
    if (_pEntry->path.IsDirectory() && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptPathDragDropPayload())
        {
            const String srcPath = ResolvePathDragDropPayload(pPayload);
            String       dstPath = _pEntry->path;
            dstPath.Join(srcPath.GetFileName(), eJoin::File);
            GetEditor().RenameFile(srcPath, dstPath);
        }

        ImGui::EndDragDropTarget();
    }

    // render filename
    {
        const SizeT len           = _pEntry->name.GetLength();
        const SizeT clipThreshold = static_cast<SizeT>(_size.x / ImGui::CalcTextSize("W").x);

        if (len <= clipThreshold)
        {
            // no clipping
            ImGui::TextUnformatted(_pEntry->name.GetPtr());
        }
        else
        {
            StackString<256> buf;
            buf.Format("{:.{}s}..", _pEntry->name.GetPtr(), clipThreshold - 1);
            ImGui::TextUnformatted(buf.GetPtr());
        }
    }

    // draw thumbnail
    {
        ImGui::SetCursorScreenPos(itemLeftTop);

        const Ref<Texture>& pThumbnail = _pEntry->pThumbnail;
        const TextureInfo&  info       = pThumbnail->GetInfo();
        const ImVec2        size       = { static_cast<float>(info.width), static_cast<float>(info.height) };
        Widget_ImageFrame(pThumbnail, size, _size);
    }

    // draw tooltip
    if (bTooltip && !bIsDragDropping && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted(_pEntry->name.GetPtr());
        ImGui::EndTooltip();
    }

    // Double click action
    if (bDoubleClicked)
    {
        if (_pEntry->path.IsDirectory())
        {
            SetFocusingDirEntry_(m_dirEntryLookUp[_pEntry->path], true);
        }
        else
        {
            OpenAsDefaultApp_(_pEntry->path);
        }
    }

    // Right click popup
    if (ImGui::BeginPopup("Menu"))
    {
        ImGui::TextUnformatted(_pEntry->name.GetPtr());

        ImGui::Separator();
        if (ImGui::BeginMenu("import"))
        {
            Menu_ImportAsset(_pEntry->path);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (Widget_MenuItem("open"))
        {
            if (_pEntry->IsDirectory())
            {
                SetFocusingDirEntry_(m_dirEntryLookUp[_pEntry->path], true);
            }
            else
            {
                OpenAsDefaultApp_(_pEntry->path);
            }
        }

        if (Widget_MenuItem("open in explorer"))
        {
            OpenInFileExplorer_(_pEntry->path, true);
        }

        if (Widget_MenuItem("rename"))
        {
            RenameFileDialogInfo info = {};
            info.title                = "rename file";
            info.srcPath              = _pEntry->path;

            GetEditor().PushDialog(MakeScoped<RenameFileDialog>(info));
        }

        if (Widget_MenuItem("delete"))
        {
            GetEditor().RemoveFile(_pEntry->path);
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::EndGroup();
    ImGui::PopStyleColor();
}

void Editor_FileBrowserWindow::Widget_UndoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanUndo());

    // button
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::FileBrowserNavUndo);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::ArrowButton("undo", ImGuiDir_Left))
    {
        const Ref<DirEntry>& pEntry = m_historyStack.Undo();
        SetFocusingDirEntry_(pEntry, false);
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "undo",
        "go back to the previous directory in history.",
        shortcut);

    ImGui::EndDisabled();
}

void Editor_FileBrowserWindow::Widget_RedoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanRedo());

    // button
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::FileBrowserNavRedo);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::ArrowButton("redo", ImGuiDir_Right))
    {
        const Ref<DirEntry>& pEntry = m_historyStack.Redo();
        SetFocusingDirEntry_(pEntry, false);
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "redo",
        "go forward to the next directory in history.",
        shortcut);

    ImGui::EndDisabled();
}

void Editor_FileBrowserWindow::Widget_BackButton_()
{
    const Ref<DirEntry>& pFocusingDirEntry = GetEditor().GetFocusingDirEntry();
    ImGui::BeginDisabled(pFocusingDirEntry == m_pRootDirNode);

    // button
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::FileBrowserNavBack);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button(".."))
    {
        GoToParentDir_();
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "back",
        "go to the parent directory.\n"
        "if drag and drop a file here, the file will be moved to the parent directory.",
        shortcut);

    // drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        // 만약 파일을 받으면, 이전 디렉토리로 파일을 이동시켜줌
        if (const ImGuiPayload* pPayload = AcceptPathDragDropPayload())
        {
            const String srcPath = ResolvePathDragDropPayload(pPayload);
            String       dstPath = pFocusingDirEntry->path;
            dstPath.Join(srcPath.GetFileName());
            GetEditor().RenameFile(srcPath, dstPath);
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowserWindow::Widget_FileEntries()
{
    const float visibleWidth = ImGui::GetContentRegionAvail().x;
    if (visibleWidth < 0.f)
    {
        return;
    }

    const ImVec2 btnSize      = kFileEntrySize * m_zoom;
    const float  pad          = kFileEntrySpacingX * m_zoom;
    const Int32  numBtnPerRow = Max<Int32>(1, static_cast<Int32>((visibleWidth + pad) / (btnSize.x + pad)));
    const Int32  numItems     = static_cast<Int32>(m_filteredFileEntries.size());
    const Int32  numRows      = (numItems + numBtnPerRow - 1) / numBtnPerRow;
    const float  itemHeight   = btnSize.y + ImGui::GetTextLineHeightWithSpacing();

    ImGuiListClipper clipper = {};
    clipper.Begin(numRows, itemHeight);
    while (clipper.Step())
    {
        for (Int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            for (Int32 i = 0; i < numBtnPerRow; ++i)
            {
                const Int32 index = row * numBtnPerRow + i;

                // skip if out of range
                if (index >= numItems)
                {
                    return;
                }

                // render file entry
                Widget_FileEntry_(m_filteredFileEntries[index], btnSize);

                // sameline except last
                if (i < numBtnPerRow - 1)
                {
                    ImGui::SameLine(0, pad);
                }
            }
        }
    }
}

void Editor_FileBrowserWindow::Widget_DirEntries()
{
    Widget_DirEntries_Recursive_(m_pRootDirNode);
}

void Editor_FileBrowserWindow::Widget_DirEntries_Recursive_(
    const Ref<DirEntry>& _pEntry)
{
    const bool bHasChild = !_pEntry->children.empty();

    // falgs
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAllColumns;
    if (!bHasChild)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (_pEntry == GetEditor().GetFocusingDirEntry())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool bIsNodeOpen     = ImGui::TreeNodeEx(_pEntry->name.GetPtr(), flags);
    const bool bRenderChildren = bHasChild && bIsNodeOpen;
    const bool bClicked        = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();

    // on drag drop target move to this directory
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptPathDragDropPayload())
        {
            const String srcPath = ResolvePathDragDropPayload(pPayload);
            String       dstPath = _pEntry->path;
            dstPath.Join(srcPath.GetFileName());
            GetEditor().RenameFile(srcPath, dstPath);
        }

        ImGui::EndDragDropTarget();
    }

    // click action
    if (bClicked)
    {
        SetFocusingDirEntry_(_pEntry, true);
    }

    // render children
    if (bRenderChildren)
    {
        for (const Ref<DirEntry>& pEntry: _pEntry->children)
        {
            Widget_DirEntries_Recursive_(pEntry);
        }

        ImGui::TreePop();
    }
}

void Editor_FileBrowserWindow::Menu_FileEntriesPanel_()
{
    const Ref<DirEntry>& pEntry          = GetEditor().GetFocusingDirEntry();
    const bool           bIsRootFocusing = pEntry == m_pRootDirNode;

    if (ImGui::BeginMenu("create"))
    {
        Menu_CreateFileEntry();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("import"))
    {
        Menu_ImportAsset(pEntry->path);
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (Widget_MenuItem("open in exploror"))
    {
        OpenInFileExplorer_(GetEditor().GetFocusingDirEntry()->path, true);
    }

    if (Widget_MenuItem("back", 0, false, !bIsRootFocusing))
    {
        GoToParentDir_();
    }
}

void Editor_FileBrowserWindow::Menu_CreateFileEntry() const
{
    const Ref<DirEntry>& pEntry = GetEditor().GetFocusingDirEntry();

    if (Widget_MenuItem("new folder"))
    {
        const String path = String::Joined(pEntry->path, "new folder", eJoin::File);
        GetEditor().CreateDir(path, FileSystem::eMakeDirectoryCollisionPolicy::Rename);
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("primitive"))
    {
        if (Widget_MenuItem("cube"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Cube;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        if (Widget_MenuItem("sphere"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Sphere;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        if (Widget_MenuItem("plane"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Plane;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        if (Widget_MenuItem("circle"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Circle;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        if (Widget_MenuItem("cone"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Cone;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        if (Widget_MenuItem("cylinder"))
        {
            PrimitiveCreationDialogInfo info = {};
            info.title                       = "create primitive";
            info.saveDirOrEmpty              = pEntry->path;
            info.type                        = ePrimitiveCreation::Cylinder;
            GetEditor().PushDialog(MakeScoped<PrimitiveCreationDialog>(info));
        }

        ImGui::EndMenu();
    }

    if (Widget_MenuItem("material"))
    {
        MaterialCreationDialogInfo info = {};
        info.title                      = "create material";
        info.dirOrEmpty                 = pEntry->path;
        GetEditor().PushDialog(MakeScoped<MaterialCreationDialog>(info));
    }
}

void Editor_FileBrowserWindow::Menu_ImportAsset(
    const StringView _path) const
{
    if (Widget_MenuItem("import model"))
    {
        ModelImportDialogInfo info = {};
        info.title                 = "import model";
        info.importPathOrEmpty     = _path;
        GetEditor().PushDialog(MakeScoped<ModelImportDialog>(info));
    }

    if (Widget_MenuItem("import image"))
    {
        ImageImportDialogInfo info = {};
        info.title                 = "import image";
        info.importPathOrEmpty     = _path;
        GetEditor().PushDialog(MakeScoped<ImageImportDialog>(info));
    }
}

void Editor_FileBrowserWindow::GatherFileEntries_()
{
    m_fileEntryLookUp.clear();

    const Ref<DirEntry>& pFocusingDirEntry = GetEditor().GetFocusingDirEntry();
    Expect<DirIter>      iter              = DirIter::Create(pFocusingDirEntry->path);
    JAM_ASSERT(iter, "Failed to create directory iterator. Path: {}", pFocusingDirEntry->path);
    for (const StringView path: iter.GetValue())
    {
        GatherFileEntry_(path);
    }

    // filtered entry force update
    m_bGatherFilteredFileEntries = true;
    m_fileEntryFilterCounterSec  = kFileEntryFilterUpdateIntervalSec;
}

Ref<FileEntry> Editor_FileBrowserWindow::GatherFileEntry_(
    const StringView _path)
{
    Ref<FileEntry> pEntry    = MakeRef<FileEntry>();
    pEntry->path             = _path;
    pEntry->name             = _path.GetFileName();
    pEntry->pThumbnail       = m_thumbnailCache.GetOrPlaceholder(_path);
    m_fileEntryLookUp[_path] = pEntry;
    return pEntry;
}

void Editor_FileBrowserWindow::GatherFilteredFileEntriesIfNeed_()
{
    if (!m_bGatherFilteredFileEntries || m_fileEntryFilterCounterSec < kFileEntryFilterUpdateIntervalSec)
    {
        return;
    }

    // filter
    m_filteredFileEntries.clear();
    for (const Ref<FileEntry>& pEntry: m_fileEntryLookUp | std::views::values)
    {
        if (m_fileEntryFilter.PassFilter(pEntry->name.GetPtr()))
        {
            m_filteredFileEntries.emplace_back(pEntry);
        }
    }

    m_fileEntryFilterCounterSec  = 0.f;
    m_bGatherFilteredFileEntries = false;
}

void Editor_FileBrowserWindow::GatherDirEntries_()
{
    m_dirEntryLookUp.clear();

    const StringView contentDir = GetApp().GetContentsDirectory();
    m_pRootDirNode              = GatherDirEntries_Recursive_(contentDir);
    if (const Ref<DirEntry>& pFocusingEntry = GetEditor().GetFocusingDirEntryOrNull())
    {
        GetEditor().SetFocusingDirEntry(m_dirEntryLookUp[pFocusingEntry->path]);
    }
    else
    {
        GetEditor().SetFocusingDirEntry(m_pRootDirNode);
    }
}

Ref<DirEntry> Editor_FileBrowserWindow::GatherDirEntries_Recursive_(
    const StringView _dir)
{
    JAM_ASSERT(_dir.IsDirectory(), "Path must be a directory.");

    Ref<DirEntry> pEntry = MakeRef<DirEntry>();
    pEntry->path         = _dir;
    pEntry->name         = _dir.GetBaseName();
    pEntry->bOpened      = false;   // 어떤 값이던 상관없음.

    Expect<DirIter> iter = DirIter::Create(_dir);
    JAM_ASSERT(iter, "Failed to create directory iterator. Path: {}", _dir);
    for (const StringView path: iter.GetValue())
    {
        if (path.IsDirectory())
        {
            pEntry->children.emplace_back(GatherDirEntries_Recursive_(path));
        }
    }

    m_dirEntryLookUp[_dir] = pEntry;
    return pEntry;
}

void Editor_FileBrowserWindow::SetFocusingDirEntry_(
    const Ref<DirEntry>& _pEntry,
    const bool           _bPushHistroyStack)
{
    JAM_ASSERT(_pEntry, "Directory entry cannot be null.");

    // no change
    const Ref<DirEntry>& pFucusingDirEntry = GetEditor().GetFocusingDirEntry();
    if (pFucusingDirEntry == _pEntry)
    {
        return;
    }

    // push history sso
    if (_bPushHistroyStack)
    {
        m_historyStack.Push(GetEditor().GetFocusingDirEntry());
    }

    // set focus dir
    GetEditor().SetFocusingDirEntry(_pEntry);
}

void Editor_FileBrowserWindow::GoToParentDir_()
{
    const Ref<DirEntry>& pEntry = GetEditor().GetFocusingDirEntry();
    JAM_ASSERT(!pEntry->IsRoot(), "Current focusing directory is root. There is no parent directory.");
    SetFocusingDirEntry_(pEntry->pParent, true);
}

void Editor_FileBrowserWindow::OnFileAddEvent_(
    const FileAddedEvent& _event)
{
    const StringView path = _event.GetPath();
    if (path.IsDirectory())
    {
        GatherDirEntries_Recursive_(path);
    }

    if (path.GetDirectory() == GetEditor().GetFocusingDirEntry()->path)
    {
        GatherFileEntry_(path);

        // 필터 즉시 수집
        m_bGatherFilteredFileEntries = true;
        m_fileEntryFilterCounterSec  = kFileEntryFilterUpdateIntervalSec;
    }
}

void Editor_FileBrowserWindow::OnFileDeleteEvent_(
    const FileDeletedEvent& _event)
{
    JAM_ASSERT(GetEditor().GetFocusingFileEntry()->path != _event.GetPath(), "Currently focusing file entry is deleted. Path: {}", _event.GetPath());

    // 만약 지금 참조중인 entry가 삭제되었다면..
    const StringView path = _event.GetPath();

    const StringView focusingDirEntryPath = GetEditor().GetFocusingDirEntry()->path;
    if (focusingDirEntryPath == path)
    {
        GoToParentDir_();   // 삭제된 디렉토리가 포커싱중이었다면, 부모 디렉토리로 이동
    }

    const StringView focusingFileEntryPath = GetEditor().GetFocusingFileEntry()->path;
    if (path == focusingFileEntryPath)
    {
        GetEditor().SetFocusingFileEntry(nullptr);   // 포커싱중인 파일이 삭제되었다면, 포커싱 해제
    }

    // lookup에서 제거
    if (path.IsDirectory())
    {
        m_dirEntryLookUp.erase(path);
    }

    // 만약 삭제된 파일이 현재 포커싱중인 디렉토리에 속한 파일이라면, 해당 entry를 lookup에서 제거
    if (path.GetDirectory() == focusingFileEntryPath)
    {
        m_fileEntryLookUp.erase(path);

        // 필터 즉시 수집
        m_bGatherFilteredFileEntries = true;
        m_fileEntryFilterCounterSec  = kFileEntryFilterUpdateIntervalSec;
    }
}

void Editor_FileBrowserWindow::OnFileRenamedEvent_(
    const FileRenamedEvent& _event)
{
    const StringView oldPath              = _event.GetOldPath();
    const StringView newPath              = _event.GetNewPath();
    const StringView focusingDirEntryPath = GetEditor().GetFocusingDirEntry()->path;

    // lookup에서 제거
    if (oldPath.IsDirectory())
    {
        Ref<DirEntry> pEntry = m_dirEntryLookUp[oldPath];
        m_dirEntryLookUp.erase(oldPath);       // old path로 lookup에서 제거
        pEntry->path              = newPath;   // entry의 path를 new path로 변경
        m_dirEntryLookUp[newPath] = pEntry;    // new path로 lookup에 추가
    }

    // 만약 이름이 변경된 파일이 현재 포커싱중인 디렉토리에 속한 파일이라면, lookup에서 제거 후 새로 수집
    if (oldPath.GetDirectory() == focusingDirEntryPath)
    {
        m_fileEntryLookUp.erase(oldPath);   // old path로 lookup에서 제거
        GatherFileEntry_(newPath);          // new path로 entry를 새로 수집

        // 필터 즉시 수집
        m_bGatherFilteredFileEntries = true;
        m_fileEntryFilterCounterSec  = kFileEntryFilterUpdateIntervalSec;
    }
}

void Editor_FileBrowserWindow::OnFileWatcherEvent_(
    const FileWatcherEvent& _event) const
{
    JAM_ASSERT(std::this_thread::get_id() != GetApp().GetMainThreadId(), "File watcher event should not be called in main thread.");

    switch (_event.type)
    {
        case eFileEvent::Added:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [this, path = _event.path] {
                    FileAddedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEvent::Deleted:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [this, path = _event.path] {
                    FileDeletedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEvent::Renamed:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [this, oldPath = _event.oldPath, newPath = _event.path] {
                    FileRenamedEvent event { oldPath, newPath };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        case eFileEvent::Modified:
        {
            GetApp().SubmitCommandToMainThread(Bind(
                [this, path = _event.path] {
                    FileModifiedEvent event { path };
                    GetApp().DispatchEvent(event);
                }));
        }
        break;

        default:
            JAM_UNREACHABLE;
    }
}

void Editor_FileBrowserWindow::OnFocusingDirEntryChanged_(
    const FocusingDirEntryChangedEvent&)
{
    GatherFileEntries_();
}

void Editor_FileBrowserWindow::OpenInFileExplorer_(
    const StringView _path,
    const bool       _bSelected) const
{
    const String cmd = String::Formatted(R"( explorer.exe {} "{}" )", _bSelected ? "/select," : "", _path);
    if (std::system(cmd.GetPtr()) != 0)
    {
        JAM_ASSERT(0, "Failed to open file in explorer. Path: {}", _path);
    }
}

void Editor_FileBrowserWindow::OpenAsDefaultApp_(
    const StringView _path) const
{
    const String cmd = String::Formatted(R"( start "" "{}" )", _path);
    if (std::system(cmd.GetPtr()) != 0)
    {
        JAM_ASSERT(0, "Failed to open file with default app. Path: {}", _path);
    }
}

}   // namespace Jam