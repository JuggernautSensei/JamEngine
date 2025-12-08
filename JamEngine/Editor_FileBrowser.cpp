#include "pch.h"

#include "Editor_FileBrowser.h"

#include "Application.h"
#include "CreateMaterialDialog.h"
#include "CreatePrimitiveDialog.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Events.h"
#include "ImportImageDialog.h"
#include "ImportModelDialog.h"
#include "Input.h"
#include "JsonUtilities.h"
#include "RemoveNotEmptyDirectoryDialog.h"
#include "RenameFileDialog.h"
#include "DialogUtilities.h"

namespace jam
{

template<typename EventType>
struct FileEventCommand
{
    void operator()()
    {
        Application& app = Application::GetInstance();
        app.DispatchEvent(event);
    }
    EventType event;
};

class JamFileWatcher : public efsw::FileWatchListener
{
public:
    void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, const efsw::Action action, std::string oldFilename) override
    {
        Application& app = Application::GetInstance();

        fs::path path = dir;
        path /= filename;

        switch (action)
        {
            case efsw::Action::Add:
            {
                FileAddedEvent event { path };
                app.SubmitCommand(FileEventCommand<FileAddedEvent> { event });
            }
            break;

            case efsw::Action::Delete:
            {
                FileDeletedEvent event { path };
                app.SubmitCommand(FileEventCommand<FileDeletedEvent> { event });
            }
            break;

            case efsw::Action::Modified:
            {
                FileModifiedEvent event { path };
                app.SubmitCommand(FileEventCommand<FileModifiedEvent> { event });
            }
            break;

            case efsw::Action::Moved:
            {
                fs::path oldPath = dir;
                oldPath /= oldFilename;

                FileRenamedEvent event { oldPath, path };
                app.SubmitCommand(FileEventCommand<FileRenamedEvent> { event });
            }
            break;

            default:
                JAM_ASSERT(0, "Should never happen!");
        }
    }
};

}   // namespace jam

namespace jam
{

void Editor_FileBrowser::Init()
{
    // thumbnail cache
    m_thumbnailCache.Init();

    // file watcher
    const fs::path& contentDir = Application::GetInstance().GetContentsDirectory();

    m_pFileWatcher       = std::make_unique<efsw::FileWatcher>();
    m_pFileWatchListener = std::make_unique<JamFileWatcher>();
    m_watchID            = m_pFileWatcher->addWatch(contentDir, m_pFileWatchListener.get(), true);
    m_pFileWatcher->watch();

    SetFocusDir_(contentDir);
}

void Editor_FileBrowser::Shutdown() const
{
    m_pFileWatcher->removeWatch(m_watchID);
}

void Editor_FileBrowser::OnEvent(Event& _event)
{
    HandleEvent<FileAddedEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_FileBrowser::OnFileAddEvent_));
    HandleEvent<FileDeletedEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_FileBrowser::OnFileDeleteEvent_));
    HandleEvent<FileModifiedEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_FileBrowser::OnFileModifiedEvent_));
    HandleEvent<FileRenamedEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_FileBrowser::OnFileRenamedEvent_));
}

void Editor_FileBrowser::DrawFileEntriesChildPopup_()
{
    if (ImGui::BeginMenu("create"))
    {
        DrawCreatePopup_();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("import"))
    {
        DrawImportPopup_();
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("open in exploror"))
        OpenFileOnFileExplorer_(m_focusDir);

    if (ImGui::MenuItem("back", nullptr, false, !m_bIsRootDirFocused))
        GoToParentDir_();
}

void Editor_FileBrowser::Widget_UndoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanUndo());

    ImGui::SetNextItemShortcut(ImGuiKey_MouseX1);
    if (ImGui::ArrowButton("##UndoButton", ImGuiDir_Left))
    {
        m_historyStack.Undo();
        SetFocusDir_(m_historyStack.Current(), false);
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::Widget_RedoButton_()
{
    ImGui::BeginDisabled(!m_historyStack.CanRedo());

    ImGui::SetNextItemShortcut(ImGuiKey_MouseX2);
    if (ImGui::ArrowButton("##RedoButton", ImGuiDir_Right))
    {
        m_historyStack.Redo();
        SetFocusDir_(m_historyStack.Current(), false);
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::Widget_BackButton_()
{
    ImGui::BeginDisabled(m_bIsRootDirFocused);

    if (ImGui::Button(".."))
    {
        GoToParentDir_();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path dir     = m_focusDir.parent_path();
            const fs::path srcPath = ResolvePathDragDropPayload(pPayload);
            const fs::path dstPath = dir / srcPath.filename();
            RenameFile_(srcPath, dstPath);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndDisabled();
}

void Editor_FileBrowser::OnImGuiRender(bool& _pOpen)
{
    if (!ImGui::Begin("file browser", &_pOpen))
    {
        ImGui::End();
        return;
    }

    // update directory tree
    if (m_bNeedUpdateDirTree)
    {
        BuildDirTree_();
        m_bNeedUpdateDirTree = false;
    }

    // update file entries
    if (m_bNeedUpdateFileEntries)
    {
        BuildFileEntries_();
        m_bNeedUpdateFileEntries = false;
    }

    // toolbar
    {
        // create
        ImGui::Button("create");
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawCreatePopup_();
            ImGui::EndPopup();
        }

        // import
        ImGui::SameLine();
        ImGui::Button("import");
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawImportPopup_();
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

        ImGui::SameLine();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##path", const_cast<std::string*>(&m_focusDir.native()), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::Separator();

    // file directory
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("file directory");

        ImGui::SetNextWindowSizeConstraints({ 100.f, -FLT_MAX }, { 400.f, FLT_MAX });
        if (ImGui::BeginChild("FileDirectory", ImVec2 { 0.f, 0.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
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
            Widget_FramePaddingAlignedTextUnformatted("filter:");

            ImGui::SameLine();
            m_fileEntryFilter.Draw("##Filter", 150.f);

            ImGui::SameLine();
            if (ImGui::Button("apply"))
                m_bNeedUpdateFileEntries = true;

            ImGui::SameLine();
            if (ImGui::Button("clear"))
            {
                m_fileEntryFilter.Clear();
                m_bNeedUpdateFileEntries = true;
            }
        }

        // refresh button
        ImGui::SameLine();
        if (ImGui::Button("refresh"))
            m_bNeedUpdateFileEntries = true;

        // zoom control
        ImGui::SameLine();
        {
            ImGui::SetNextItemWidth(150.f);
            Widget_DragPercent("##Zoom", &m_zoom, 0.5f, k_minZoomPerc, k_maxZoomPerc, "zoom: %.0f%%");
        }

        if (ImGui::BeginChild("FileEntriesChild", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Border))
        {
            DrawFileEntries_();
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                DrawFileEntriesChildPopup_();
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::End();
}

Json Editor_FileBrowser::OnSerialize() const
{
    Json json;
    json["zoom"] = m_zoom;
    return json;
}

void Editor_FileBrowser::OnDeserialize(const Json& _json)
{
    try
    {
        m_zoom = _json.at("zoom").get<float>();
    }
    catch (const std::exception& e)
    {
        Log::Warn(JAM_PRETTY_FUNCTION " failed to deserialize: {}", e.what());
    }
}

void Editor_FileBrowser::DrawFileEntries_()
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

    ImGuiListClipper clipper;
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
                    return;

                // render file entry
                const FileEntry& entry = m_fileEntries[idx];
                Widget_FileEntry_(entry, btnSize);

                // sameline except last
                if (i < numBtnPerRow - 1)
                    ImGui::SameLine(0, pad);
            }
        }
    }
}

void Editor_FileBrowser::BuildDirTree_()
{
    const fs::path& rootPath = Application::GetInstance().GetContentsDirectory();
    m_rootDirNode            = BuildDirNode_Recursive_(rootPath);
}

void Editor_FileBrowser::UpdateDirNode_Recursive_(DirNode& _node)
{
    _node.bIsFocused = (_node.path == m_focusDir);
    for (DirNode& child: _node.children)
        UpdateDirNode_Recursive_(child);
}

Editor_FileBrowser::DirNode Editor_FileBrowser::BuildDirNode_Recursive_(const fs::path& _path)
{
    DirNode node;
    node.name       = _path.filename();
    node.path       = _path;
    node.bIsFocused = (node.path == m_focusDir);

    for (const fs::directory_entry& entry: fs::directory_iterator(_path))
    {
        const fs::path& path = entry.path();
        if (entry.is_directory())
            node.children.emplace_back(BuildDirNode_Recursive_(path));
    }
    return node;
}

void Editor_FileBrowser::UpdateDirTree_()
{
    UpdateDirNode_Recursive_(m_rootDirNode);
}

void Editor_FileBrowser::Widget_DirNode_Recursive_(const DirNode& _node)
{
    bool bHasChild = !_node.children.empty();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAllColumns;
    if (!bHasChild)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (_node.bIsFocused)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool bIsNodeOpen = ImGui::TreeNodeEx(_node.name.c_str(), flags);
    bIsNodeOpen &= bHasChild;

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        SetFocusDir_(_node.path);
    }

    if (bIsNodeOpen)
    {
        for (const DirNode& node: _node.children)
        {
            Widget_DirNode_Recursive_(node);
        }
        ImGui::TreePop();
    }
}

void Editor_FileBrowser::Widget_FileEntry_(const FileEntry& _entry, const ImVec2 _size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::BeginGroup();
    ImGui::PushID(_entry.name.c_str());

    // draw button
    const ImVec2 itemLeftTop = ImGui::GetCursorScreenPos();
    ImGui::Button("##fileEntry", _size);
    ImGui::OpenPopupOnItemClick("Menu");

    const bool bTooltip        = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    const bool bDoubleClicked  = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    bool       bIsDragDropping = false;

    // drag drop source
    if (ImGui::BeginDragDropSource())
    {
        // set payload
        SetFileDragDropPayload(_entry.path);

        // preview
        ImGui::TextUnformatted(_entry.name.c_str());
        ImGui::EndDragDropSource();
    }

    // drag drop target (only directory)
    if (_entry.bIsDir && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path srcPath = ResolvePathDragDropPayload(pPayload);
            const fs::path dstPath = _entry.path / srcPath.filename();
            RenameFile_(srcPath, dstPath);
        }

        bIsDragDropping = true;
        ImGui::EndDragDropTarget();
    }

    // draw text
    {
        const std::string& displayName = _entry.name;
        const Int32        nameLen     = NarrowCast<Int32>(displayName.size());
        const Int32        maxChars    = static_cast<Int32>(Div(_size.x, ImGui::CalcTextSize("W").x));

        if (nameLen <= maxChars)
        {
            ImGui::TextUnformatted(displayName.c_str());
        }
        else
        {
            const char* shortName = FormatStringToTmpBuffer("%.*s..", maxChars - 1, displayName.c_str());
            ImGui::TextUnformatted(shortName);
        }
    }

    // draw thumbnail
    {
        const Ref<Thumbnail>& pThumbnail = _entry.pThumbnail;

        const Resolution res       = pThumbnail->GetResolution();
        const float      aspect    = static_cast<float>(res.width) / static_cast<float>(res.height);
        const ImVec2     imgaeSize = aspect >= 1.f ? ImVec2 { _size.x, _size.x / aspect } : ImVec2 { _size.y * aspect, _size.y };

        const ImVec2 offset = (_size - imgaeSize) * 0.5f;
        const ImVec2 p0     = itemLeftTop + offset;
        const ImVec2 p1     = p0 + imgaeSize;
        ImGui::GetWindowDrawList()->AddImage(pThumbnail->GetImTexturRef(), p0, p1);
    }

    // draw tooltip
    if (bTooltip && !bIsDragDropping && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted(_entry.name.c_str());
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
        ImGui::TextUnformatted(_entry.name.c_str());

        ImGui::Separator();
        if (ImGui::BeginMenu("import"))
        {
            DrawImportFromFileEntryPopup_(_entry.path);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("open"))
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

        if (ImGui::MenuItem("open in explorer"))
            OpenFileOnFileExplorer_(_entry.path);

        if (ImGui::MenuItem("rename"))
            Editor::OpenCorePopupDialog("rename file", RenameFileDialog { _entry.path });

        if (ImGui::MenuItem("delete"))
            DeleteFile_(_entry.path);

        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::EndGroup();
    ImGui::PopStyleColor();
}

void Editor_FileBrowser::DrawCreatePopup_() const
{
    if (ImGui::MenuItem("new folder"))
        CreateNewFolder_(m_focusDir);

    ImGui::Separator();

    if (ImGui::BeginMenu("primitive"))
    {
        if (ImGui::MenuItem("cube"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Cube });

        if (ImGui::MenuItem("sphere"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Sphere });

        if (ImGui::MenuItem("plane"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Plane });

        if (ImGui::MenuItem("circle"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Circle });

        if (ImGui::MenuItem("cone"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Cone });

        if (ImGui::MenuItem("cylinder"))
            Editor::OpenCorePopupDialog("create mesh", CreatePrimitiveDialog { m_focusDir, eMeshType::Cylinder });

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("material"))
        Editor::OpenCorePopupDialog("create material", CreateMaterialDialog { m_focusDir });
}

void Editor_FileBrowser::DrawImportPopup_() const
{
    if (ImGui::MenuItem("model"))
        Editor::OpenCorePopupDialog("import model", ImportModelDialog { m_focusDir });

    if (ImGui::MenuItem("image"))
        Editor::OpenCorePopupDialog("import image", ImportImageDialog { m_focusDir });
}

void Editor_FileBrowser::DrawImportFromFileEntryPopup_(const fs::path& _path) const
{
    if (ImGui::MenuItem("import model"))
        Editor::OpenCorePopupDialog("import model", ImportModelDialog { _path });

    if (ImGui::MenuItem("import image"))
        Editor::OpenCorePopupDialog("import image", ImportImageDialog { _path });
}

void Editor_FileBrowser::BuildFileEntries_()
{
    m_fileEntries.clear();
    for (const fs::directory_entry& entry: fs::directory_iterator(m_focusDir))
    {
        const fs::path& path     = entry.path();
        const fs::path  filename = path.filename();

        if (m_fileEntryFilter.PassFilter(filename.c_str()))
        {
            m_fileEntries.emplace_back(filename, path, entry.is_directory(), m_thumbnailCache.LoadThumbnail(path));
        }
    }
}

void Editor_FileBrowser::SetFocusDir_(const fs::path& _dir, const bool _bPushHistory)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path is not a directory!");

    if (m_focusDir == _dir)
    {
        return;
    }

    // set focus dir
    m_focusDir = _dir;

    // is child of contents dir
    const fs::path& contentsDir = Application::GetInstance().GetContentsDirectory();
    JAM_ASSERT(IsAncestorPathOf(contentsDir, m_focusDir) || contentsDir == m_focusDir, "Focused directory is not a child of contents directory!");

    // history stack
    if (_bPushHistory)
    {
        m_historyStack.Push(m_focusDir);
    }

    // need update flags
    m_bIsRootDirFocused      = (contentsDir == m_focusDir);
    m_bNeedUpdateFileEntries = true;

    // update focused dir node
    UpdateDirTree_();
}

void Editor_FileBrowser::GoToParentDir_()
{
    SetFocusDir_(m_focusDir.parent_path());
}

void Editor_FileBrowser::OnDragDropTargetBackButton_() const
{
}

void Editor_FileBrowser::OnFileAddEvent_(const FileAddedEvent& _event)
{
    m_bNeedUpdateDirTree     = true;
    m_bNeedUpdateFileEntries = true;
}

void Editor_FileBrowser::OnFileDeleteEvent_(const FileDeletedEvent& _event)
{
    m_bNeedUpdateDirTree     = true;
    m_bNeedUpdateFileEntries = true;
}

void Editor_FileBrowser::OnFileModifiedEvent_(const FileModifiedEvent& _event)
{
    m_bNeedUpdateDirTree     = true;
    m_bNeedUpdateFileEntries = true;
}

void Editor_FileBrowser::OnFileRenamedEvent_(const FileRenamedEvent& _event)
{
    m_bNeedUpdateDirTree     = true;
    m_bNeedUpdateFileEntries = true;
}

void Editor_FileBrowser::RenameFile_(const fs::path& _src, const fs::path& _dst) const
{
    try
    {
        fs::rename(_src, _dst);
    }
    catch (const fs::filesystem_error& e)
    {
        std::string msg = std::format("failed to rename file from '{}' to '{}'.\n{}", _src.native(), _dst.native(), e.what());
        Editor::OpenCorePopupDialog("error", msg);
    }
}

void Editor_FileBrowser::OpenFileAsDefaultApplication_(const fs::path& _path) const
{
    if (!OpenFileWithDefaultApplication(_path))
    {
        std::string msg = std::format("failed to open file '{}'.\n{}", _path.native(), GetErrorString());
        Editor::OpenCorePopupDialog("error", msg);
    }
}

void Editor_FileBrowser::OpenFileOnFileExplorer_(const fs::path& _path) const
{
    if (!OpenFileExplorer(_path))
    {
        std::string msg = std::format("failed to open file explorer at '{}'.\n{}", _path.native(), GetErrorString());
        Editor::OpenCorePopupDialog("error", msg);
    }
}

void Editor_FileBrowser::DeleteFile_(const fs::path& _path) const
{
    bool bHasAnyFile = false;
    if (fs::is_directory(_path))
    {
        for (const fs::directory_entry& _: fs::directory_iterator(_path))
        {
            bHasAnyFile = true;
            break;
        }
    }

    // if directory is not empty, show confirmation dialog
    if (bHasAnyFile)
    {
        Editor::OpenCorePopupDialog("remove not empty directory", RemoveNotEmptyDirectoryDialog { _path });
    }
    else
    {
        // remove regular file or empty directory
        try
        {
            fs::remove(_path);
        }
        catch (const fs::filesystem_error& e)
        {
            std::string msg = std::format("failed to remove file: {}\n{}", _path.native(), e.what());
            Editor::OpenCorePopupDialog("error", msg);
        }
    }
}

void Editor_FileBrowser::CreateNewFolder_(const fs::path& _parentDir) const
{
    JAM_ASSERT(fs::is_directory(_parentDir), "Parent path is not a directory!");

    const fs::path& contentDir = Application::GetInstance().GetContentsDirectory();
    JAM_ASSERT(IsAncestorPathOf(contentDir, _parentDir) || contentDir == _parentDir, "Parent directory is not a child of contents directory!");

    for (Int32 attempt = 0; attempt < 1000; ++attempt)
    {
        // create path
        fs::path path = _parentDir / "new folder";
        if (attempt > 0)
            path += FormatStringToTmpBuffer(" (%d)", attempt);

        if (!fs::exists(path))
        {
            try
            {
                fs::create_directory(path);
            }
            catch (const fs::filesystem_error& e)
            {
                std::string msg = std::format("failed to create new folder in '{}'.\n{}", _parentDir.native(), e.what());
                Editor::OpenCorePopupDialog("error", msg);
            }

            return;
        }
    }

    // failed
    std::string msg = std::format("failed to create new folder in '{}'. too many folders with the same name exist.", _parentDir.native());
    Editor::OpenCorePopupDialog("error", msg);
}

}   // namespace jam