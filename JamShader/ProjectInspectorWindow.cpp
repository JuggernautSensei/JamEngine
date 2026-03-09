#include "pch.h"
#include "ProjectInspectorWindow.h"

#include "Events.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"

void ProjectInspectorWindow::OnImGuiRender()
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    if (!ImGui::Begin("project inspector"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("ProjectInspector", 2))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("basename", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_ProjectName_();

        ImGui::TableNextColumn();
        Widget_Text("working directory", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_ReadOnlyInputText("##workingDir", m_pProject->GetWorkingDirectory());

        ImGui::EndTable();
    }

    // shader items + 4 buttons
    ImGui::NewLine();
    Widget_Text("shader items", true);

    constexpr ImVec2 k_btnSize = { 100.f, 0.f };
    constexpr Int32  k_numBtns = 3;
    ImGuiStyle&      style     = ImGui::GetStyle();
    const float      totalW    = k_btnSize.x * k_numBtns + style.ItemSpacing.x * (k_numBtns - 1);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2 { ImGui::GetContentRegionAvail().x - totalW, 0.f });
    if (ImGui::Button("add", k_btnSize))
    {
        NewShaderItem_();
    }

    ImGui::BeginDisabled(m_pFocusingShaderItem == nullptr);
    ImGui::SameLine();
    if (ImGui::Button("clone", k_btnSize))
    {
        CloneFocisingShaderItem_();
    }

    ImGui::SameLine();
    if (ImGui::Button("remove", k_btnSize))
    {
        RemoveFocisingShaderItem_();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("clear", k_btnSize))
    {
        ClearShaderItems_();
    }

    // shader item list
    if (ImGui::BeginListBox("##ShaderItemList", ImGui::GetContentRegionAvail() - ImVec2 { 0.f, ImGui::GetTextLineHeightWithSpacing() }))
    {
        for (const auto& [name, item]: m_pProject->GetShaderItems())
        {
            bool bSelected = m_pFocusingShaderItem == &item;
            if (ImGui::Selectable(name.GetPtr(), bSelected))
            {
                SetFocusingShaderItem_(name);
            }
        }

        ImGui::EndListBox();
    }

    // num of shader items
    ImGui::AlignTextToFramePadding();
    ImGui::Text("total: %d", m_pProject->GetShaderItems().GetSize());

    ImGui::End();
}

void ProjectInspectorWindow::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher(_event);
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ProjectInspectorWindow::OnProjectChangedEvent_);
}

void ProjectInspectorWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent& _event)
{
    m_pProject          = _event.GetProjectOrNull();
    m_projectRenameEdit = m_pProject ? m_pProject->GetBaseName() : StringView {};
    SetFocusingShaderItem_({});
}

void ProjectInspectorWindow::Widget_ProjectName_()
{
    Widget_InputText("##ProjectNameEdit", m_projectRenameEdit);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_pProject->Rename(m_projectRenameEdit);
    }
}

void ProjectInspectorWindow::NewShaderItem_()
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    String name = "new shader item";
    for (Int32 i = 0; m_pProject->ContainsShaderItem(name); ++i)
    {
        name = String::Formatted("new shader item {}", i);
    }

    m_pProject->AddShaderItem(name, {});
    SetFocusingShaderItem_(name);
}

void ProjectInspectorWindow::CloneFocisingShaderItem_()
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    String name = m_focusingShaderItemName;
    for (Int32 i = 0; m_pProject->ContainsShaderItem(name); ++i)
    {
        name = String::Formatted("{} {}", m_focusingShaderItemName, i);
    }

    const ShaderItem& focusingItem = m_pProject->GetShaderItem(m_focusingShaderItemName);
    m_pProject->AddShaderItem(name, focusingItem);
    SetFocusingShaderItem_(name);
}

void ProjectInspectorWindow::RemoveFocisingShaderItem_()
{
    m_pProject->RemoveShaderItem(m_focusingShaderItemName);
    SetFocusingShaderItem_({});
}

void ProjectInspectorWindow::ClearShaderItems_() const
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");
}

void ProjectInspectorWindow::SetFocusingShaderItem_(
    const StringView _nameOrEmpty)
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    if (m_focusingShaderItemName == _nameOrEmpty)
    {
        return;
    }

    m_focusingShaderItemName = _nameOrEmpty;
    m_pFocusingShaderItem    = _nameOrEmpty.IsEmpty() ? nullptr : &m_pProject->GetShaderItem(_nameOrEmpty);
    FocusingShaderItemChagnedEvent event { _nameOrEmpty };
    GetApp().DispatchEvent(event);
}
