#include "pch.h"
#include "NewProjectDialog.h"

#include "GUIFileSystem.h"
#include "MainScene.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/JsonArchive.h"

namespace
{

[[nodiscard]] FileDialog CreateFileDialog_()
{
    OpenDirectoryDialogInfo info = {};
    info.title                   = "select project directory";
    return FileDialog { info };
}

}   // namespace

NewProjectDialog::NewProjectDialog(
    const NewProjectDialogInfo& _info)
    : Dialog(_info.title, _info.size)
    , m_dir(_info.dir)
    , m_dlg(CreateFileDialog_())
{
    JAM_ASSERT(m_dir.IsDirectory() || m_dir.IsEmpty(), "dir must be a directory or empty");
}

void NewProjectDialog::OnShow()
{
    ImGui::Text("project directory:");
    SetNextItemToAvailWidth();
    Widget_FileDialog("##directory", m_dir, m_dlg);

    ImGui::Text("project name:");
    SetNextItemToAvailWidth();
    Widget_InputText("##projectName", m_baseName);

    const ImVec2 btnSize = CalcFitButtonSize(2);
    ImGui::Spacing();
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

bool NewProjectDialog::OnClickCreateButton_() const
{
    if (m_dir.IsEmpty() || m_baseName.IsEmpty())
    {
        GetMainScene().PushMessageDialog(
            "invalid input",
            "project directory and project name cannot be empty.");

        return false;
    }

    String path = String::Joined(m_dir, m_baseName, eJoinType::Directory);

    // create directory
    if (!GUI::FS::MakeDirectory(path, FileSystem::eMakeDirectoryCollisionPolicy::Fail))
    {
        return false;
    }

    // create empty project
    path.Join(m_baseName, eJoinType::File);
    path.ReplaceExtension(Project::k_extension);
    path = FileSystem::Absolute(path);

    const Expect<Ref<Project>> pProject = Project::CreateNew(path);
    if (!pProject)
    {
        GetMainScene().PushMessageDialog(
            "failed to create project",
            String::Formatted("failed to create project at path: {}", path));

        return false;
    }

    GetMainScene().SetProject(pProject.GetValue());
    return true;
}