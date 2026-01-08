#include "pch.h"

#include "Editor_FileBrowser.h"

#include "Application.h"
#include "ConfirmationDialog.h"
#include "MaterialCreationDialog.h"
#include "PrimitiveCreationDialog.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Events.h"
#include "ImageImportDialog.h"
#include "ModelImportDialog.h"
#include "RenameFileDialog.h"
#include "WindowsUtilities.h"

namespace jam
{

class JamFileWatcher : public efsw::FileWatchListener
{
public:
    void handleFileAction(
        efsw::WatchID      watchid,
        const std::string& dir,
        const std::string& filename,
        const efsw::Action action,
        std::string        oldFilename) override
    {
        Application& app = Application::GetInstance();

        fs::path path = dir;
        path /= filename;

        switch (action)
        {
            case efsw::Action::Add:
            {
                FileAddedEvent event { path };
                app.SubmitCommand(JAM_BIND_METHOD(this, JamFileWatcher::HandleFileAction<FileAddedEvent>, event));
            }
            break;

            case efsw::Action::Delete:
            {
                FileDeletedEvent event { path };
                app.SubmitCommand(JAM_BIND_METHOD(this, JamFileWatcher::HandleFileAction<FileDeletedEvent>, event));
            }
            break;

            case efsw::Action::Modified:
            {
                FileModifiedEvent event { path };
                app.SubmitCommand(JAM_BIND_METHOD(this, JamFileWatcher::HandleFileAction<FileModifiedEvent>, event));
            }
            break;

            case efsw::Action::Moved:
            {
                fs::path oldPath = dir;
                oldPath /= oldFilename;

                FileRenamedEvent event { oldPath, path };
                app.SubmitCommand(JAM_BIND_METHOD(this, JamFileWatcher::HandleFileAction<FileRenamedEvent>, event));
            }
            break;

            default:
                JAM_ASSERT(0, "Should never happen!");
        }
    }

private:
    template<typename EventType>
    void HandleFileAction(EventType _event)
    {
        Application& app = Application::GetInstance();
        app.DispatchEvent(_event);
    }
};

}   // namespace jam

namespace jam
{

void Editor_FileBrowser::Init()
{
    const Application& app         = Application::GetInstance();
    const fs::path&    contentsDir = app.GetContentsDirectory();

    // file watcher
    {
        m_pFileWatcher       = MakeScoped<efsw::FileWatcher>();
        m_pFileWatchListener = MakeScoped<JamFileWatcher>();
        m_watchID            = m_pFileWatcher->addWatch(contentsDir, m_pFileWatchListener.GetPtr(), true);
        m_pFileWatcher->watch();
    }

    SetFocusDir_(contentsDir, false);
}

void Editor_FileBrowser::Shutdown() const
{
    m_pFileWatcher->removeWatch(m_watchID);
}

void Editor_FileBrowser::OnEvent(Event& _event)
{
    HandleEvent<FileAddedEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_FileBrowser::OnFileAddEvent_));
    HandleEvent<FileDeletedEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_FileBrowser::OnFileDeleteEvent_));
    HandleEvent<FileModifiedEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_FileBrowser::OnFileModifiedEvent_));
    HandleEvent<FileRenamedEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_FileBrowser::OnFileRenamedEvent_));
    m_thumbnailCache.OnEvent(_event);
}

void Editor_FileBrowser::Menu_FileEntriesPanel()
{
    if (ImGui::BeginMenu("create"))
    {
        Menu_CreateFileEntry();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("import"))
    {
        Menu_ImportAsset(m_focusedDir);
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (Widget_MenuItem("open in exploror"))
    {
        OpenFileOnFileExplorer_(m_focusedDir);
    }

    if (Widget_MenuItem("back", 0, false, !m_bIsRootDirFocused))
    {
        GoToParentDir_();
    }
}

void Editor_FileBrowser::Widget_UndoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanUndo());

    // button
    ButtonDesc desc = {};
    desc.text       = "undo";
    desc.direction  = ImGuiDir_Left;
    desc.shortcut   = ImGuiKey_MouseX1;
    if (Widget_Button(desc))
    {
        const fs::path path = m_historyStack.Undo();
        SetFocusDir_(path, false);
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::Widget_RedoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanRedo());

    // button
    ButtonDesc desc = {};
    desc.text       = "undo";
    desc.direction  = ImGuiDir_Right;
    desc.shortcut   = ImGuiKey_MouseX2;
    if (Widget_Button(desc))
    {
        const fs::path path = m_historyStack.Redo();
        SetFocusDir_(path, false);
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::Widget_BackButton_()
{
    ImGui::BeginDisabled(m_bIsRootDirFocused);

    // button
    ButtonDesc desc = {};
    desc.text       = "..";
    desc.shortcut   = ImGuiKey_ModAlt | ImGuiKey_LeftArrow;
    if (Widget_Button(desc))
    {
        GoToParentDir_();
    }

    // drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        // 만약 파일을 받으면, 이전 디렉토리로 파일을 이동시켜줌
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path srcPath = ResolvePathDragDropPayload(pPayload);
            MoveFile_(srcPath, m_focusedDir.parent_path());
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::OnImGuiRender(
    bool& _pOpen)
{
    if (!ImGui::Begin("file browser", &_pOpen))
    {
        ImGui::End();
        return;
    }

    // update directory tree
    if (m_bNeedBuildDirTree)
    {
        BuildDirTree_();
        m_bNeedBuildDirTree = false;
    }

    // update file entries
    if (m_bNeedGatherFileEntries && m_fileEntryGatheringCounterSec >= k_fileEntryGatheringDebounceSec)
    {
        GatherFileEntries_();
        m_fileEntryGatheringCounterSec = 0.f;
        m_bNeedGatherFileEntries       = false;
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
            Menu_ImportAsset(m_focusedDir);
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
        SetNextItemAvailWidth();
        ImGui::InputText("##path", const_cast<std::string*>(&m_focusedDir.native()), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::Separator();

    // file directory
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("directories");

        ImGui::SetNextWindowSizeConstraints({ 200.f, -FLT_MAX }, { 400.f, FLT_MAX });
        if (ImGui::BeginChild("directories", ImVec2 { 0.f, 0.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
        {
            Widget_DirTree_();
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
            Widget_FrameAlignedText("filter:");

            ImGui::SameLine();
            if (m_fileEntryFilter.Draw("##Filter", 150.f))
            {
                m_bNeedGatherFileEntries = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("clear"))
            {
                m_fileEntryFilter.Clear();
                m_fileEntryGatheringCounterSec = 0.f;
                m_bNeedGatherFileEntries       = true;
            }
        }

        // refresh button
        ImGui::SameLine();
        if (ImGui::Button("refresh"))
        {
            m_fileEntryGatheringCounterSec = 0.f;
            m_bNeedGatherFileEntries       = true;
        }

        // zoom control
        ImGui::SameLine();
        {
            ImGui::SetNextItemWidth(150.f);
            Widget_DragPercent("##Zoom", m_zoom, 0.5f, k_minZoomPerc, k_maxZoomPerc, "zoom: %.0f%%");
        }

        // file entries area
        if (ImGui::BeginChild("FileEntriesPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Border))
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

void Editor_FileBrowser::Serialize(JsonWriter _writer)
{
    _writer.WriteField("zoom", m_zoom);
}

void Editor_FileBrowser::Deserialize(JsonReader _reader)
{
    _reader.ReadField("zoom", m_zoom);
}

void Editor_FileBrowser::Widget_FileEntries()
{
    const float displayWidth = ImGui::GetContentRegionAvail().x;
    if (displayWidth < 0.f)
    {
        return;
    }

    constexpr ImVec2 k_itemSize = { 100.f, 100.f };
    constexpr float  k_itemPad  = 20.f;

    const ImVec2 btnSize      = k_itemSize * m_zoom;
    const float  pad          = k_itemPad * m_zoom;
    const Int32  numBtnPerRow = Max(1, static_cast<Int32>(Div(displayWidth + pad, btnSize.x + pad)));
    const Int32  numItems     = NarrowCast<Int32>(m_fileEntries.size());
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

void Editor_FileBrowser::BuildDirTree_()
{
    const Application& app      = Application::GetInstance();
    const fs::path&    rootPath = app.GetContentsDirectory();
    m_rootDirNode               = BuildDirNode_Recursive_(rootPath);
}

Editor_FileBrowser::DirNode Editor_FileBrowser::BuildDirNode_Recursive_(
    const fs::path& _path)
{
    DirNode node = { _path.filename(), _path, {}, _path == m_focusedDir };
    for (const fs::directory_entry& entry: fs::directory_iterator(_path))
    {
        if (entry.is_directory())
        {
            const fs::path& path = entry.path();
            node.children.emplace_back(BuildDirNode_Recursive_(path));
        }
    }
    return node;
}

void Editor_FileBrowser::MarkFocuedDirNode_()
{
    MarkFocusedDirNode_Recursive_(m_rootDirNode);
}

bool Editor_FileBrowser::MarkFocusedDirNode_Recursive_(
    DirNode& _node)
{
    if (_node.path == m_focusedDir)
    {
        _node.bIsFocused = true;
        return true;
    }
    else
    {
        for (DirNode& child: _node.children)
        {
            if (MarkFocusedDirNode_Recursive_(child))
            {
                return true;
            }
        }

        return false;
    }
}

void Editor_FileBrowser::Widget_DirNode_Recursive_(
    const DirNode& _node)
{
    const bool bHasChild = !_node.children.empty();

    // falgs
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAllColumns;
    if (!bHasChild)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (_node.bIsFocused)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool bIsNodeOpen     = ImGui::TreeNodeEx(_node.name.c_str(), flags);
    const bool bRenderChildren = bHasChild && bIsNodeOpen;
    const bool bClicked        = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();

    // on drag drop target move to this directory
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path acceptedPath = ResolvePathDragDropPayload(pPayload);
            MoveFile_(acceptedPath, _node.path);
        }

        ImGui::EndDragDropTarget();
    }

    // click action
    if (bClicked)
    {
        SetFocusDir_(_node.path);
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

void Editor_FileBrowser::Widget_FileEntry_(
    const FileEntry& _entry,
    const ImVec2     _size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0);   // invisible button bg
    ImGui::BeginGroup();
    ImGui::PushID(_entry.filename.c_str());

    // draw button
    const ImVec2 startPos = ImGui::GetCursorScreenPos();
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
        ImGui::TextUnformatted(_entry.filename.c_str());
        ImGui::EndDragDropSource();
    }

    // drag drop target (디렉토리인 경우만. accept path의 parent dir을 자신으로 변경)
    if (_entry.bIsDir && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path srcPath = ResolvePathDragDropPayload(pPayload);
            MoveFile_(srcPath, _entry.path);
        }

        ImGui::EndDragDropTarget();
    }

    // render filename
    {
        const std::string& text          = _entry.filename;
        const Int32        len           = NarrowCast<Int32>(text.size());
        const Int32        clipThreshold = static_cast<Int32>(_size.x / ImGui::CalcTextSize("W").x);

        if (len <= clipThreshold)
        {
            // no clipping
            ImGui::TextUnformatted(text.c_str());
        }
        else
        {
            StringView shortText = GlobalBuffer::FormatTmp("{.{}}..", text, clipThreshold - 1);
            ImGui::TextUnformatted(shortText.GetPtr());
        }
    }

    // draw thumbnail
    {
        const Ref<ImageTexture>& pThumbnail = _entry.pThumbnail;

        const Vec2i  res       = pThumbnail->GetResolution();
        const float  aspect    = static_cast<float>(res.x) / static_cast<float>(res.y);
        const ImVec2 imageSize = aspect >= 1.f ? ImVec2 { _size.x, _size.x / aspect } : ImVec2 { _size.y * aspect, _size.y };

        const ImVec2 offset = (_size - imageSize) * 0.5f;
        const ImVec2 p0     = startPos + offset;
        const ImVec2 p1     = p0 + imageSize;

        ImGui::GetWindowDrawList()->AddImage(ImGuiGlue::ImTextureRefFrom(pThumbnail), p0, p1);
    }

    // draw tooltip
    if (bTooltip && !bIsDragDropping && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted(_entry.filename.c_str());
        ImGui::EndTooltip();
    }

    // Double click action
    if (bDoubleClicked)
    {
        if (_entry.bIsDir)
        {
            SetFocusDir_(_entry.path);
        }
        else
        {
            OpenFileAsDefaultApplication_(_entry.path);
        }
    }

    // Right click popup
    if (ImGui::BeginPopup("Menu"))
    {
        ImGui::TextUnformatted(_entry.filename.c_str());

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
                SetFocusDir_(_entry.path);
            }
            else
            {
                OpenFileAsDefaultApplication_(_entry.path);
            }
        }

        if (Widget_MenuItem("open in explorer"))
        {
            OpenFileOnFileExplorer_(_entry.path);
        }

        if (Widget_MenuItem("rename"))
        {
            Editor::PushDialog(MakeScoped<RenameFileDialog>(_entry.path));
        }

        if (Widget_MenuItem("delete"))
        {
            DeleteFile_(_entry.path);
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::EndGroup();
    ImGui::PopStyleColor();
}

void Editor_FileBrowser::Widget_DirTree_()
{
    Widget_DirNode_Recursive_(m_rootDirNode);
}

void Editor_FileBrowser::Menu_CreateFileEntry() const
{
    if (Widget_MenuItem("new folder"))
        CreateFolder_(m_focusedDir);

    ImGui::Separator();

    if (ImGui::BeginMenu("primitive"))
    {
        if (Widget_MenuItem("cube"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Cube));

        if (Widget_MenuItem("sphere"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Sphere));

        if (Widget_MenuItem("plane"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Plane));

        if (Widget_MenuItem("circle"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Circle));

        if (Widget_MenuItem("cone"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Cone));

        if (Widget_MenuItem("cylinder"))
            Editor::PushDialog(MakeScoped<PrimitiveCreationDialog>(m_focusedDir, ePrimitiveCreationType::Cylinder));

        ImGui::EndMenu();
    }

    if (Widget_MenuItem("material"))
        Editor::PushDialog(MakeScoped<MaterialCreationDialog>(m_focusedDir));
}

void Editor_FileBrowser::Menu_ImportAsset(
    const fs::path& _path) const
{
    if (Widget_MenuItem("import model"))
        Editor::PushDialog(MakeScoped<ModelImportDialog>(_path));

    if (Widget_MenuItem("import image"))
        Editor::PushDialog(MakeScoped<ImageImportDialog>(_path));
}

void Editor_FileBrowser::GatherFileEntries_()
{
    m_fileEntries.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(m_focusedDir))
    {
        const fs::path& path     = entry.path();
        const fs::path  filename = path.filename();

        if (m_fileEntryFilter.PassFilter(filename.c_str()))
        {
            m_fileEntries.emplace_back(filename, path, entry.is_directory(), m_thumbnailCache.GetOrPlaceholder(path));
        }
    }
}

void Editor_FileBrowser::SetFocusDir_(
    const fs::path& _dir,
    const bool      _bPushHistroyStack)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path is not a directory!");

    // no change
    if (m_focusedDir == _dir)
    {
        return;
    }

    // set focus dir
    const fs::path oldPath = m_focusedDir;
    m_focusedDir           = _dir;

    // is child of contents dir
    const Application& app         = Application::GetInstance();
    const fs::path&    contentsDir = app.GetContentsDirectory();
    JAM_ASSERT(IsBasePathOf(contentsDir, m_focusedDir) || contentsDir == m_focusedDir, "Focused directory is not a child of contents directory!");

    // history stack
    if (_bPushHistroyStack && !oldPath.empty())
    {
        m_historyStack.Push(oldPath);
    }

    // need update flags
    m_bIsRootDirFocused      = (contentsDir == m_focusedDir);
    m_bNeedGatherFileEntries = true;

    // update focused dir node
    MarkFocuedDirNode_();
}

void Editor_FileBrowser::GoToParentDir_()
{
    SetFocusDir_(m_focusedDir.parent_path());
}

void Editor_FileBrowser::OnFileAddEvent_(const FileAddedEvent& _event)
{
    m_bNeedBuildDirTree      = true;
    m_bNeedGatherFileEntries = true;
}

void Editor_FileBrowser::OnFileDeleteEvent_(const FileDeletedEvent& _event)
{
    m_bNeedBuildDirTree      = true;
    m_bNeedGatherFileEntries = true;
}

void Editor_FileBrowser::OnFileModifiedEvent_(const FileModifiedEvent& _event)
{
    m_bNeedBuildDirTree      = true;
    m_bNeedGatherFileEntries = true;
}

void Editor_FileBrowser::OnFileRenamedEvent_(const FileRenamedEvent& _event)
{
    m_bNeedBuildDirTree      = true;
    m_bNeedGatherFileEntries = true;
}

void Editor_FileBrowser::MoveFile_(
    const fs::path& _path,
    const fs::path& _newDir) const
{
    JAM_ASSERT(fs::is_directory(_newDir), "New directory path is not a directory!");

    const fs::path patn = _newDir / _path.filename();

    // rename
    std::error_code ec = {};
    fs::rename(_path, patn, ec);
    if (ec)
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to move file from '{}' to '{}'.\n{}", _path.native(), patn.native(), ec.message());
        Editor::PushMessageDialog("error", msg);
    }
}

void Editor_FileBrowser::DeleteFile_(
    const fs::path& _path) const
{
    bool bHasAnyFile = false;
    for (const fs::directory_entry& _: fs::directory_iterator(_path))
    {
        bHasAnyFile = true;
        break;
    }

    if (bHasAnyFile)
    {
        const StringView msg = GlobalBuffer::FormatTmp("the directory '{}' is not empty. are you sure you want to remove it and all its contents?", _path.native());
        Editor::PushDialog(MakeScoped<ConfirmationDialog>("remove not empty directory", msg, JAM_BIND_METHOD(this, Editor_FileBrowser::OnClickRemoveNotEmptyDirectoryYesButton_, _path)));
    }
    else
    {
        std::error_code ec = {};
        fs::remove(_path, ec);
        if (ec)
        {
            Editor::PushMessageDialog(
                "error",
                GlobalBuffer::FormatTmp("failed to remove file '{}'.\n{}", _path.native(), ec.message()));
        }
    }
}

void Editor_FileBrowser::CreateFolder_(
    const fs::path& _parentDir) const
{
    constexpr Int32 k_attemptLimit = 1000;

    for (Int32 i = 0; i < k_attemptLimit; ++i)
    {
        StringView filename = {};
        if (i == 0)   // first attempt
        {
            filename = GlobalBuffer::FormatTmp("new folder");
        }
        else
        {
            filename = GlobalBuffer::FormatTmp("new folder ({})", i);
        }

        const fs::path  dirPath = _parentDir / filename.GetPtr();
        std::error_code ec      = {};
        if (fs::create_directory(dirPath, ec))
        {
            return;   // succeeded
        }

        // error occured
        if (ec)
        {
            const StringView msg = GlobalBuffer::FormatTmp("failed to create new folder in '{}'.\n{}", _parentDir.native(), ec.message());
            Editor::PushMessageDialog("error", msg);
            return;
        }

        // already exists. retry
    }

    // no error but too many folders with same name exist
    const StringView msg = GlobalBuffer::FormatTmp("failed to create new folder in '{}'. too many folders with the same name exist.", _parentDir.native());
    Editor::PushMessageDialog("error", msg);
}

void Editor_FileBrowser::OnClickRemoveNotEmptyDirectoryYesButton_(
    const fs::path& _dir) const
{
    std::error_code ec = {};
    fs::remove_all(_dir, ec);
    if (ec)
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to remove directory '{}'.\n{}", _dir.native(), ec.message());
        Editor::PushMessageDialog("error", msg);
    }
}

void Editor_FileBrowser::OpenFileAsDefaultApplication_(const fs::path& _path) const
{
    if (!OpenFileAsDefaultApplication(_path))
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to open file '{}' with default application. {}", _path.native(), GlobalBuffer::GetLastError());
        Editor::PushMessageDialog("error", msg);
    }
}

void Editor_FileBrowser::OpenFileOnFileExplorer_(const fs::path& _path) const
{
    if (!OpenFileExplorer(_path))
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to open file explorer at '{}'. {}", _path.native(), GlobalBuffer::GetLastError());
        Editor::PushMessageDialog("error", msg);
    }
}

}   // namespace jam