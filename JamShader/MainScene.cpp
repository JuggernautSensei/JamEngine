#include "pch.h"

#include "MainScene.h"

#include "../JamEngine/ConfirmationDialog.h"
#include "../JamEngine/MessageDialog.h"

namespace
{

class ShaderAppFileWatcherListener : public FileWatchListener
{
public:
    void handleFileAction(
        efsw::WatchID      _watchid,
        const std::string& _dir,
        const std::string& _filename,
        const efsw::Action _action,
        const std::string  _oldFilename) override
    {
        switch (_action)
        {
            case efsw::Action::Add:
            {
                JAM_APP.SubmitCommandToMainThread(Bind(
                    [_dir, _filename]
                    {
                        FileAddedEvent event { Path { _dir } / _filename };
                        JAM_APP.DispatchEvent(event);
                    }));
            }
            break;
            case efsw::Action::Delete:
            {
                JAM_APP.SubmitCommandToMainThread(Bind(
                    [_dir, _filename]
                    {
                        FileDeletedEvent event { Path { _dir } / _filename };
                        JAM_APP.DispatchEvent(event);
                    }));
            }
            break;
            case efsw::Action::Modified:
            {
                JAM_APP.SubmitCommandToMainThread(Bind(
                    [_dir, _filename]
                    {
                        FileModifiedEvent event { Path { _dir } / _filename };
                        JAM_APP.DispatchEvent(event);
                    }));
            }
            break;
            case efsw::Action::Moved:
            {
                JAM_APP.SubmitCommandToMainThread(Bind(
                    [_dir, _filename, _oldFilename]
                    {
                        FileRenamedEvent event { Path { _dir } / _oldFilename, Path { _dir } / _filename };
                        JAM_APP.DispatchEvent(event);
                    }));
            }
            break;
            default:
                JAM_UNREACHABLE;
        }
    }
};

}   // namespace

MainScene::MainScene()
    : m_resourcePath(JAM_APP.GetWorkingDirectory() / k_resourceDirName)
{
    fs::create_directories(m_resourcePath);

    m_pFileWatcher         = MakeScoped<FileWatcher>();
    m_pFileWatcherListener = MakeScoped<ShaderAppFileWatcherListener>();
    m_fileWatcherID        = m_pFileWatcher->addWatch(m_resourcePath.native(), m_pFileWatcherListener.GetPtr(), true);
    m_pFileWatcher->watch();
}

void MainScene::OnEnter()
{
}

void MainScene::OnRender(const RenderView& _desc)
{
    JAM_GFX.ClearColor(_desc.fbh, RGBA::k_black);
}

void MainScene::OnImGuiRender()
{
    BuildFileEntriesIfNeed_();
    Panel_FileBrowser_();
    m_dialogStack.Show();
}

void MainScene::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileAddedEvent>(this, &MainScene::OnFileAddedEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &MainScene::OnFileDeletedEvent_);
    dispatcher.Dispatch<FileModifiedEvent>(this, &MainScene::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileRenamedEvent>(this, &MainScene::OnFileRenamedEvent_);
}

void MainScene::OnFileAddedEvent_(
    const FileAddedEvent&)
{
    m_bFileEntriesDirty = true;
}

void MainScene::OnFileDeletedEvent_(
    const FileDeletedEvent&)
{
    m_bFileEntriesDirty = true;
}

void MainScene::OnFileModifiedEvent_(
    const FileModifiedEvent&)
{
    m_bFileEntriesDirty = true;
}

void MainScene::OnFileRenamedEvent_(
    const FileRenamedEvent&)
{
    m_bFileEntriesDirty = true;
}

void MainScene::CreateNewDirectory_(
    const Path&      _dir,
    const StringView _nameHintOrEmpty)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path must be a directory");

    ScratchBuffer buf;
    StringView    baseName = _nameHintOrEmpty.IsEmpty() ? "NewDirectory" : _nameHintOrEmpty;
    buf.Format("{}", baseName);

    Path path = _dir / PathFrom(buf.ToStringView());
    for (Int32 i = 0; i < 1000; ++i)
    {
        if (!fs::exists(path))
        {
            break;
        }

        buf.Format("{}_{}", baseName, i);
        path = _dir / PathFrom(buf.ToStringView());
    }

    std::error_code ec = {};
    fs::create_directories(path, ec);
    if (ec)
    {
        buf.Format("failed to create directory \"{}\". {}.", path, ec.message());

        MessageDialogInfo info = {};
        info.baseInfo.title    = "create directory failed";
        info.msg               = buf.ToStringView();
        m_dialogStack.Push(MakeRef<MessageDialog>(info));
    }
}

void MainScene::CreateNewHLSLFile_(
    const Path&      _dir,
    const StringView _nameHintOrEmpty)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path must be a directory");

    ScratchBuffer buf;
    StringView    baseName = _nameHintOrEmpty.IsEmpty() ? "NewShader" : _nameHintOrEmpty;
    buf.Format("{}.hlsl", baseName);

    Path path = _dir / PathFrom(buf.ToStringView());
    for (Int32 i = 0; i < 1000; ++i)
    {
        if (!fs::exists(path))
        {
            break;
        }

        buf.Format("{}_{}.hlsl", baseName, i);
        path = _dir / PathFrom(buf.ToStringView());
    }

    Expect<FileWriter> writer = FileWriter::Open(path, { eSaveFileFlags::CreateDirs, eSaveFileFlags::FailIfExists });
    if (!writer)
    {
        buf.Format("failed to create file \"{}\". {}.", path, writer.GetError().ToErrorMessage());

        MessageDialogInfo info = {};
        info.baseInfo.title    = "create file failed";
        info.msg               = buf.ToStringView();
        m_dialogStack.Push(MakeRef<MessageDialog>(info));
    }
}

void MainScene::RemoveFile_(
    const fs::path& _path)
{
    if (fs::is_directory(_path) && !fs::is_empty(_path))
    {
        ScratchBuffer buf;
        buf.Format("failed to delete directory \"{}\". Directory is not empty.", _path);

        ConfirmationDialogInfo info = {};
        info.baseInfo.title         = "delete directory failed";
        info.msg                    = buf.ToStringView();
        info.OnConfirm              = Bind(
            [this, _path]
            {
                RemoveNonEmptyDirectory_(_path);
            });

        m_dialogStack.Push(MakeRef<ConfirmationDialog>(info));
        return;
    }

    std::error_code ec = {};
    fs::remove(_path, ec);
    if (ec)
    {
        ScratchBuffer buf;
        buf.Format("failed to delete file \"{}\". {}.", _path, ec.message());
        MessageDialogInfo info = {};
        info.baseInfo.title    = "delete file failed";
        info.msg               = buf.ToStringView();
        m_dialogStack.Push(MakeRef<MessageDialog>(info));
    }
}

void MainScene::RemoveNonEmptyDirectory_(const fs::path& _dir)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path must be a directory");
    JAM_ASSERT(!fs::is_empty(_dir), "Directory must not be empty");

    std::error_code ec = {};
    fs::remove_all(_dir, ec);
    if (ec)
    {
        ScratchBuffer buf;
        buf.Format("failed to delete directory \"{}\". {}.", _dir, ec.message());

        MessageDialogInfo info = {};
        info.baseInfo.title    = "delete directory failed";
        info.msg               = buf.ToStringView();
        m_dialogStack.Push(MakeRef<MessageDialog>(info));
    }
}

void MainScene::BuildFileEntriesIfNeed_()
{
    if (!m_bFileEntriesDirty)
    {
        return;
    }

    m_fileEntryLut.clear();
    m_rootFileEntry                      = BuildFileEntry_Recursive_(m_resourcePath);
    m_fileEntryLut[m_rootFileEntry.path] = &m_rootFileEntry;
    if (!m_focusingPath.empty() && m_fileEntryLut.contains(m_focusingPath))
    {
        m_focusingPath = m_fileEntryLut[m_focusingPath]->path;
    }

    m_bFileEntriesDirty = false;
}

MainScene::FileEntry MainScene::BuildFileEntry_Recursive_(
    const Path& _path)
{
    if (fs::is_directory(_path))
    {
        FileEntry fileEntry = {};
        fileEntry.path      = _path;
        fileEntry.name      = FileNameViewOf(fileEntry.path);
        fileEntry.bIsDir    = true;
        for (const fs::directory_entry& child: fs::directory_iterator { fileEntry.path })
        {
            fileEntry.children.push_back(BuildFileEntry_Recursive_(child.path()));
            m_fileEntryLut[child.path()] = &fileEntry.children.back();
        }
        return fileEntry;
    }
    else
    {
        FileEntry fileEntry = {};
        fileEntry.path      = _path;
        fileEntry.name      = FileNameViewOf(fileEntry.path);
        fileEntry.bIsDir    = false;
        return fileEntry;
    }
}

void MainScene::Panel_FileBrowser_()
{
    if (!ImGui::Begin("file browser"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginChild("FileEntriesPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        Widget_FileEntry_Recursive_(m_rootFileEntry);
    }
    ImGui::EndChild();

    ImGui::End();
}

void MainScene::Widget_FileEntry_Recursive_(
    const FileEntry& _entry)
{
    const bool         bSelected = m_focusingPath == _entry.path;
    ImGuiTreeNodeFlags flags     = ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (bSelected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!_entry.bIsDir)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool bOpened = ImGui::TreeNodeEx(_entry.name.GetPtr(), flags);
    if (ImGui::IsItemClicked())
    {
        m_focusingPath = _entry.path;
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("delete"))
        {
            RemoveFile_(_entry.path);
        }

        ImGui::EndPopup();
    }

    if (bOpened)
    {
        for (const FileEntry& child: _entry.children)
        {
            Widget_FileEntry_Recursive_(child);
        }
        ImGui::TreePop();
    }
}

void MainScene::Menu_CreateFileEntry_(
    const Path&      _dir,
    const StringView _nameHintOrEmpty)
{
    JAM_ASSERT(fs::is_directory(_dir), "Path must be a directory");

    if (ImGui::MenuItem("create new hlsl file"))
    {
        CreateNewHLSLFile_(_dir, _nameHintOrEmpty);
    }

    if (ImGui::MenuItem("create new directory"))
    {
        CreateNewDirectory_(_dir, _nameHintOrEmpty);
    }
}
