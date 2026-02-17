#include "pch.h"

#include "Editor_FileBrowserWindow.h"

#include "Application.h"
#include "MaterialCreationDialog.h"
#include "PrimitiveCreationDialog.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Events.h"
#include "ImageImportDialog.h"
#include "ModelImportDialog.h"
#include "RenameFileDialog.h"

namespace Jam
{

class JamFileWatcher : public efsw::FileWatchListener
{
public:
    void handleFileAction(
        efsw::WatchID      watchid,
        const std::string& dir,
        const std::string& filename,
        const efsw::Action action,
        const std::string  oldFilename) override
    {
        Path path = dir;
        path /= filename;

        switch (action)
        {
            case efsw::Action::Add:
            {
                auto func = [path]
                {
                    FileAddedEvent event { path };
                    JAM_APP.DispatchEvent(event);
                };

                JAM_APP.SubmitCommandToMainThread(Bind(std::move(func)));
            }
            break;

            case efsw::Action::Delete:
            {
                auto func = [path]
                {
                    FileDeletedEvent event { path };
                    JAM_APP.DispatchEvent(event);
                };

                JAM_APP.SubmitCommandToMainThread(Bind(std::move(func)));
            }
            break;

            case efsw::Action::Modified:
            {
                auto func = [path]
                {
                    FileModifiedEvent event { path };
                    JAM_APP.DispatchEvent(event);
                };

                JAM_APP.SubmitCommandToMainThread(Bind(std::move(func)));
            }
            break;

            case efsw::Action::Moved:
            {
                Path oldPath = dir;
                oldPath /= oldFilename;

                auto func = [oldPath, path]
                {
                    FileRenamedEvent event { oldPath, path };
                    JAM_APP.DispatchEvent(event);
                };

                JAM_APP.SubmitCommandToMainThread(Bind(std::move(func)));
            }
            break;

            default:
                JAM_UNREACHABLE;
        }
    }
};

}   // namespace Jam

namespace Jam
{

Editor_FileBrowserWindow::Editor_FileBrowserWindow()
{
    const Path& contentsDir = JAM_APP.GetContentsDirectory();

    // file watcher
    {
        m_pFileWatcher       = MakeScoped<efsw::FileWatcher>();
        m_pFileWatchListener = MakeScoped<JamFileWatcher>();
        m_watchID            = m_pFileWatcher->addWatch(contentsDir, m_pFileWatchListener.GetPtr(), true);
        m_pFileWatcher->watch();
    }

    SetFocusingDir_(contentsDir, false);
}

Editor_FileBrowserWindow::~Editor_FileBrowserWindow()
{
    m_pFileWatcher->removeWatch(m_watchID);
}

void Editor_FileBrowserWindow::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileAddedEvent>(this, &Editor_FileBrowserWindow::OnFileAddEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &Editor_FileBrowserWindow::OnFileDeleteEvent_);
    dispatcher.Dispatch<FileModifiedEvent>(this, &Editor_FileBrowserWindow::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileRenamedEvent>(this, &Editor_FileBrowserWindow::OnFileRenamedEvent_);

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

    // update directory tree
    if (m_bBuildDirTreeRequired)
    {
        BuildDirTree_();
        m_bBuildDirTreeRequired = false;
    }

    // update file entries
    if (m_bGatherFileEntriesRequried && m_fileEntryGatheringCounterSec >= k_fileEntryGatheringDebounceSec)
    {
        GatherFileEntries_();
        m_fileEntryGatheringCounterSec = 0.f;
        m_bGatherFileEntriesRequried   = false;
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
            Menu_ImportAsset(m_focusingDir);
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
        ImGui::InputText("##path", const_cast<std::string*>(&m_focusingDir.native()), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::Separator();

    // file directory
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("directories");

        ImGui::SetNextWindowSizeConstraints({ 200.f, 0.f }, { 400.f, Limits<float>::k_max });
        if (ImGui::BeginChild("directories", ImVec2 { 0.f, 0.f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
        {
            Widget_DirNode_Recursive_(m_rootDirNode);
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    // file entries
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        // filter input
        {
            // update gathering time for debounce
            const float dt = ImGui::GetIO().DeltaTime;
            m_fileEntryGatheringCounterSec += dt;

            // render filter input
            Widget_Text("filter:", true);

            ImGui::SameLine();
            if (m_fileEntryFilter.Draw("##Filter", 150.f))
            {
                m_bGatherFileEntriesRequried = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("clear"))
            {
                m_bGatherFileEntriesRequried = true;
            }
        }

        // refresh button
        ImGui::SameLine();
        if (ImGui::Button("refresh"))
        {
            m_fileEntryGatheringCounterSec = 0.f;
            m_bGatherFileEntriesRequried   = true;
        }

        // zoom control
        ImGui::SameLine();
        {
            ImGui::SetNextItemWidth(150.f);
            Widget_DragPercent("##Zoom", m_zoom, 0.5f, k_minZoomPerc, k_maxZoomPerc, "zoom: %.0f%%");
        }

        // file entries area
        if (ImGui::BeginChild("FileEntriesPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
        {
            // draw file entries
            Widget_FileEntries();

            // menu
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                Menu_FileEntriesPanel();
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
    const FileEntry& _entry,
    const ImVec2     _size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0);   // invisible button bg
    ImGui::BeginGroup();
    ImGui::PushID(_entry.name.GetPtr());

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
        SetFileDragDropPayload(_entry.path);

        // preview
        ImGui::TextUnformatted(_entry.name.GetPtr());
        ImGui::EndDragDropSource();
    }

    // drag drop target (디렉토리인 경우만. accept path의 parent dir을 자신으로 변경)
    if (_entry.bIsDir && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path srcPath = ResolvePathDragDropPayload(pPayload);
            JAM_EDITOR.ReplaceFileDir(srcPath, _entry.path);
        }

        ImGui::EndDragDropTarget();
    }

    // render filename
    {
        const SizeT len           = _entry.name.GetLength();
        const SizeT clipThreshold = NumericCast<SizeT>(_size.x / ImGui::CalcTextSize("W").x);

        if (len <= clipThreshold)
        {
            // no clipping
            ImGui::TextUnformatted(_entry.name.GetPtr());
        }
        else
        {
            ScratchBuffer buf;
            buf.Format("{:.{}s}..", _entry.name.GetPtr(), clipThreshold - 1);
            ImGui::TextUnformatted(buf.GetPtr());
        }
    }

    // draw thumbnail
    {
        ImGui::SetCursorScreenPos(itemLeftTop);

        const Ref<Texture>& pThumbnail = _entry.pThumbnail;
        const Vec2i         res        = pThumbnail->GetResolution();
        const ImVec2        imageSize  = ImVec2From(res);
        Widget_ImageFrame(pThumbnail, imageSize, _size);
    }

    // draw tooltip
    if (bTooltip && !bIsDragDropping && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted(_entry.name.GetPtr());
        ImGui::EndTooltip();
    }

    // Double click action
    if (bDoubleClicked)
    {
        if (_entry.bIsDir)
        {
            SetFocusingDir_(_entry.path);
        }
        else
        {
            OpenFileWithDefaultApp_(_entry.path);
        }
    }

    // Right click popup
    if (ImGui::BeginPopup("Menu"))
    {
        ImGui::TextUnformatted(_entry.name.GetPtr());

        ImGui::Separator();
        if (ImGui::BeginMenu("import"))
        {
            Menu_ImportAsset(_entry.path);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (Widget_MenuItem("open"))
        {
            if (_entry.bIsDir)
            {
                SetFocusingDir_(_entry.path);
            }
            else
            {
                OpenFileWithDefaultApp_(_entry.path);
            }
        }

        if (Widget_MenuItem("open in explorer"))
        {
            OpenFileInExplorer_(_entry.path, true);
        }

        if (Widget_MenuItem("rename"))
        {
            RenameFileDialogDesc desc = {};
            desc.baseInfo.title       = "rename file";
            desc.srcPath              = _entry.path;

            JAM_EDITOR.PushDialog(MakeRef<RenameFileDialog>(desc));
        }

        if (Widget_MenuItem("delete"))
        {
            JAM_EDITOR.RemoveFile(_entry.path);
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
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::FileBrowserNavUndo));
    if (ImGui::ArrowButton("undo", ImGuiDir_Left))
    {
        const Path path = m_historyStack.Undo();
        SetFocusingDir_(path, false);
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "undo",
        "go back to the previous directory in history.");

    ImGui::EndDisabled();
}

void Editor_FileBrowserWindow::Widget_RedoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanRedo());

    // button
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::FileBrowserNavRedo));
    if (ImGui::ArrowButton("redo", ImGuiDir_Right))
    {
        const Path path = m_historyStack.Redo();
        SetFocusingDir_(path, false);
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "redo",
        "go forward to the next directory in history.");

    ImGui::EndDisabled();
}

void Editor_FileBrowserWindow::Widget_BackButton_()
{
    ImGui::BeginDisabled(m_rootDirNode.bIsFocusing);

    // button
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::FileBrowserNavBack));
    if (ImGui::Button(".."))
    {
        GoToParentDir_();
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "back",
        "go to the parent directory.\n"
        "if drag and drop a file here, the file will be moved to the parent directory.");

    // drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        // 만약 파일을 받으면, 이전 디렉토리로 파일을 이동시켜줌
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path srcPath = ResolvePathDragDropPayload(pPayload);
            const Path newDir  = m_focusingDir.parent_path();
            JAM_EDITOR.ReplaceFileDir(srcPath, newDir);
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

    const ImVec2 btnSize      = k_fileEntrySize * m_zoom;
    const float  pad          = k_fileEntrySpacingX * m_zoom;
    const Int32  numBtnPerRow = Max<Int32>(1, NumericCast<Int32>(Div(visibleWidth + pad, btnSize.x + pad)));
    const Int32  numItems     = NumericCast<Int32>(m_fileEntries.size());
    const Int32  numRows      = Div(numItems + numBtnPerRow - 1, numBtnPerRow);
    const float  itemHeight   = btnSize.y + ImGui::GetTextLineHeightWithSpacing();

    ImGuiListClipper clipper = {};
    clipper.Begin(numRows, itemHeight);
    while (clipper.Step())
    {
        for (Int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            for (Int32 i = 0; i < numBtnPerRow; ++i)
            {
                const Int32 idx = row * numBtnPerRow + i;

                // skip if out of range
                if (idx >= numItems)
                {
                    return;
                }

                // render file entry
                const FileEntry& entry = m_fileEntries[idx];
                Widget_FileEntry_(entry, btnSize);

                // sameline except last
                if (i < numBtnPerRow - 1)
                {
                    ImGui::SameLine(0, pad);
                }
            }
        }
    }
}

void Editor_FileBrowserWindow::Widget_DirNode_Recursive_(
    const DirNode& _node)
{
    const bool bHasChild = !_node.children.empty();

    // falgs
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAllColumns;
    if (!bHasChild)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (_node.bIsFocusing)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool bIsNodeOpen     = ImGui::TreeNodeEx(_node.name.GetPtr(), flags);
    const bool bRenderChildren = bHasChild && bIsNodeOpen;
    const bool bClicked        = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();

    // on drag drop target move to this directory
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path acceptedPath = ResolvePathDragDropPayload(pPayload);
            JAM_EDITOR.ReplaceFileDir(acceptedPath, _node.path);
        }

        ImGui::EndDragDropTarget();
    }

    // click action
    if (bClicked)
    {
        SetFocusingDir_(_node.path);
    }

    // render children
    if (bRenderChildren)
    {
        for (const DirNode& node: _node.children)
        {
            Widget_DirNode_Recursive_(node);
        }

        ImGui::TreePop();
    }
}

void Editor_FileBrowserWindow::Menu_FileEntriesPanel()
{
    if (ImGui::BeginMenu("create"))
    {
        Menu_CreateFileEntry();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("import"))
    {
        Menu_ImportAsset(m_focusingDir);
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (Widget_MenuItem("open in exploror"))
    {
        OpenFileInExplorer_(m_focusingDir, true);
    }

    if (Widget_MenuItem("back", 0, false, !m_rootDirNode.bIsFocusing))
    {
        GoToParentDir_();
    }
}

void Editor_FileBrowserWindow::Menu_CreateFileEntry() const
{
    if (Widget_MenuItem("new folder"))
    {
        JAM_EDITOR.CreateNewDir(m_focusingDir);
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("primitive"))
    {
        if (Widget_MenuItem("cube"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Cube;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        if (Widget_MenuItem("sphere"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Sphere;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        if (Widget_MenuItem("plane"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Plane;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        if (Widget_MenuItem("circle"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Circle;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        if (Widget_MenuItem("cone"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Cone;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        if (Widget_MenuItem("cylinder"))
        {
            PrimitiveCreationDialogDesc desc = {};
            desc.baseInfo.title              = "create primitive";
            desc.saveDirOrEmpty              = m_focusingDir;
            desc.type                        = ePrimitiveCreationType::Cylinder;

            JAM_EDITOR.PushDialog(MakeRef<PrimitiveCreationDialog>(desc));
        }

        ImGui::EndMenu();
    }

    if (Widget_MenuItem("material"))
    {
        MaterialCreationDialogDesc desc = {};
        desc.baseInfo.title             = "create material";
        desc.dirOrEmpty                 = m_focusingDir;

        JAM_EDITOR.PushDialog(MakeRef<MaterialCreationDialog>(desc));
    }
}

void Editor_FileBrowserWindow::Menu_ImportAsset(
    const Path& _path) const
{
    if (Widget_MenuItem("import model"))
    {
        ModelImportDialogInfo desc = {};
        desc.baseInfo.title        = "import model";
        desc.importPathOrEmpty     = _path;

        JAM_EDITOR.PushDialog(MakeRef<ModelImportDialog>(desc));
    }

    if (Widget_MenuItem("import image"))
    {
        ImageImportDialogInfo desc = {};
        desc.baseInfo.title        = "import image";
        desc.importPathOrEmpty     = _path;

        JAM_EDITOR.PushDialog(MakeRef<ImageImportDialog>(desc));
    }
}

void Editor_FileBrowserWindow::GatherFileEntries_()
{
    m_fileEntries.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(m_focusingDir))
    {
        const Path&      path     = entry.path();
        const StringView filename = FileNameViewOf(path);

        if (m_fileEntryFilter.PassFilter(filename.GetPtr()))
        {
            m_fileEntries.emplace_back(path, filename, entry.is_directory(), m_thumbnailCache.GetOrPlaceholder(path));
        }
    }
}

void Editor_FileBrowserWindow::BuildDirTree_()
{
    const Path& root = JAM_APP.GetContentsDirectory();
    m_rootDirNode    = BuildDirNode_Recursive_(root);
}

Editor_FileBrowserWindow::DirNode Editor_FileBrowserWindow::BuildDirNode_Recursive_(
    const Path& _path)
{
    DirNode node     = {};
    node.path        = _path;
    node.name        = FileNameViewOf(node.path);
    node.bIsFocusing = (m_focusingDir == node.path);

    // iterate sub directories. and build child nodes
    for (const fs::directory_entry& entry: fs::directory_iterator { _path })
    {
        if (entry.is_directory())
        {
            const Path& path = entry.path();
            node.children.emplace_back(BuildDirNode_Recursive_(path));
        }
    }

    return node;
}

bool Editor_FileBrowserWindow::MarkFocusingDirNode_Recursive_(
    DirNode& _node)
{
    if (_node.path == m_focusingDir)
    {
        _node.bIsFocusing = true;
        return true;
    }

    for (DirNode& node: _node.children)
    {
        if (MarkFocusingDirNode_Recursive_(node))
        {
            return true;
        }
    }

    return false;
}

void Editor_FileBrowserWindow::SetFocusingDir_(
    const Path& _dir,
    const bool  _bPushHistroyStack)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path is not a directory!");
    if (JAM_DEBUG_ENABLED)
    {
        const Path& root = JAM_APP.GetContentsDirectory();
        JAM_ASSERT(IsBasePathOf(root, m_focusingDir) || root == m_focusingDir, "Focused directory is not a child of contents directory!");
    }

    // no change
    if (m_focusingDir == _dir)
    {
        return;
    }

    // push history stack
    if (_bPushHistroyStack && !m_focusingDir.empty())
    {
        m_historyStack.Push(m_focusingDir);
    }

    // set focus dir
    m_focusingDir                = _dir;
    m_bGatherFileEntriesRequried = true;
    MarkFocusingDirNode_Recursive_(m_rootDirNode);
}

void Editor_FileBrowserWindow::GoToParentDir_()
{
    SetFocusingDir_(m_focusingDir.parent_path());
}

void Editor_FileBrowserWindow::OnFileAddEvent_(
    const FileAddedEvent& _event)
{
    OnFileEvent_(_event.GetPath());
}

void Editor_FileBrowserWindow::OnFileDeleteEvent_(
    const FileDeletedEvent& _event)
{
    OnFileEvent_(_event.GetPath());
}

void Editor_FileBrowserWindow::OnFileModifiedEvent_(
    const FileModifiedEvent& _event)
{
    OnFileEvent_(_event.GetPath());
}

void Editor_FileBrowserWindow::OnFileRenamedEvent_(
    const FileRenamedEvent& _event)
{
    OnFileEvent_(_event.GetOldPath());   // 하나만 처리해도 충분
}

void Editor_FileBrowserWindow::OnFileEvent_(
    const Path& _path)
{
    if (fs::is_directory(_path))
    {
        m_bBuildDirTreeRequired = true;
    }
    else if (fs::is_regular_file(_path))
    {
        if (_path.parent_path() == m_focusingDir)
        {
            m_bGatherFileEntriesRequried = true;
        }
    }
}

void Editor_FileBrowserWindow::OpenFileInExplorer_(
    const Path& _path,
    const bool  _bSelected) const
{
    ScratchBuffer buf;
    buf.Format(R"( explorer.exe {} "{}" )", _bSelected ? "/select," : "", _path);

    const Int32 ret = std::system(buf.GetPtr());
    JAM_ASSERT(ret == 0, "Failed to open file in explorer. Return code: {}", ret);
}

void Editor_FileBrowserWindow::OpenFileWithDefaultApp_(
    const Path& _path) const
{
    ScratchBuffer command;
    command.Format(R"( start "" "{}" )", _path);

    const Int32 ret = std::system(command.GetPtr());
    JAM_ASSERT(ret == 0, "Failed to open file with default application. Return code: {}", ret);
}

}   // namespace Jam