#include "pch.h"
#include "NewProjectDialog.h"

#include "GUIFileSystem.h"
#include "ShaderApp.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"

namespace
{

[[nodiscard]] FileDialog CreateFileDialog_()
{
    OpenDirectoryDialogInfo info = {};
    info.title                   = "select project directory";
    return FileDialog { info };
}

}   // namespace

NewProjectDialog::NewProjectDialog()
    : Dialog("new project")
    , m_dlg(CreateFileDialog_())
{
}

void NewProjectDialog::OnShow()
{
    // directory
    ImGui::Text("project directory:");
    SetNextItemToAvailWidth();
    Widget_FileDialog("##directory", m_dir, m_dlg);

    // project typeName
    ImGui::Text("project typeName:");
    SetNextItemToAvailWidth();
    Widget_InputText("##projectName", m_name);

    // final buttons
    ImGui::Spacing();
    SetCursorScreenPosFromWindowBottom(ImGui::GetFrameHeight());
    const ImVec2 btnSize = CalcFitButtonSize(2);
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
    if (m_dir.IsEmpty() || m_name.IsEmpty())
    {
        GetShaderApp().PushMessageDialog("invalid input", "project directory and project typeName cannot be empty.");
        return false;
    }

    // directory / typeName / typeName.extension

    // create working directory
    String workingDir = String::Joined(m_dir, m_name, eJoin::Directory);
    if (!GUI::MakeDirectory(workingDir, FileSystem::eMakeDirectoryCollisionPolicy::Fail))
    {
        return false;
    }

    // create empty project
    workingDir.Join(m_name, eJoin::File);
    workingDir.ReplaceExtension(Project::k_extension);

    const Expect<Project> project = Project::LoadNew(workingDir);
    if (!project)
    {
        GetShaderApp().PushMessageDialog("failed to create project", String::Formatted("failed to create project at path: {}", workingDir));
        return false;
    }

    GetShaderApp().SetProject(project.GetValue());
    return true;
}