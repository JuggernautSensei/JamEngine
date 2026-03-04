#include "pch.h"
#include "WorkspaceWindow.h"

#include "Events.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"

void WorkspaceWindow::OnImGuiRender()
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    if (!ImGui::Begin("workspace"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("ProjectInspector", 2))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("name", true);
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
    float            totalW    = k_btnSize.x * k_numBtns + ImGui::GetStyle().ItemSpacing.x * (k_numBtns - 1);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2 { ImGui::GetContentRegionAvail().x - totalW, 0.f });
    if (ImGui::Button("add", k_btnSize))
    {
        NewShaderItem_();
    }

    ImGui::BeginDisabled(GetMainScene().GetFocusingShaderItemOrNull() == nullptr);
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
    ImVec2 shaderItemListSize = ImGui::GetContentRegionAvail();
    shaderItemListSize.y -= ImGui::GetTextLineHeightWithSpacing();
    if (ImGui::BeginListBox("##ShaderItemList", shaderItemListSize))
    {
        const Ref<ShaderItem>& pFocusingShaderItem = GetMainScene().GetFocusingShaderItemOrNull();
        for (const Ref<ShaderItem>& pShaderItem: m_pProject->GetShaders())
        {
            bool bSelected = (pFocusingShaderItem == pShaderItem);
            if (ImGui::Selectable(pShaderItem->GetName().GetPtr(), bSelected))
            {
                GetMainScene().SetFocusingShaderItem(pShaderItem);
            }
        }

        ImGui::EndListBox();
    }

    // num of shader items
    ImGui::AlignTextToFramePadding();
    ImGui::Text("total: %d", m_pProject->GetShaders().size());

    ImGui::End();
}

void WorkspaceWindow::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher(_event);
    dispatcher.Dispatch<ProjectChangedEvent>(this, &WorkspaceWindow::OnProjectChangedEvent_);
}

void WorkspaceWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent& _event)
{
    m_pProject = _event.GetProjectOrNull();
}

void WorkspaceWindow::Widget_ProjectName_()
{
    if (m_bRenamingProject)
    {
        Widget_InputText("##ProjectNameEdit", m_renameProjectBuffer);

        if (ImGui::IsItemDeactivated())
        {
            m_pProject->Rename(m_renameProjectBuffer);
            m_bRenamingProject = false;
        }
    }
    else
    {
        Widget_ReadOnlyInputText("##ProjectNameReadOnly", m_pProject->GetName());

        if (ImGui::IsItemActivated())
        {
            m_renameProjectBuffer = m_pProject->GetName();
            m_bRenamingProject    = true;
        }
    }
}

void WorkspaceWindow::NewShaderItem_() const
{
    Ref<ShaderItem> pShaderItem = MakeRef<ShaderItem>();
    pShaderItem->SetName("new shader");

    GetMainScene().GetProject()->GetMainScene().SetFocusingShaderItem(pShaderItem);
}

void WorkspaceWindow::CloneFocisingShaderItem_() const
{
    JAM_ASSERT(GetMainScene().GetFocusingShaderItemOrNull(), "No shader item is currently focused.");

    const Ref<ShaderItem>& pFocusingShaderItem = GetMainScene().GetFocusingShaderItemOrNull();
    const Ref<ShaderItem>  pShaderItem         = MakeRef<ShaderItem>(*pFocusingShaderItem);
    GetMainScene().GetProject()->PushShader(pShaderItem, eAddShaderCollisionPolicy::Rename);
    GetMainScene().SetFocusingShaderItem(pShaderItem);
}

void WorkspaceWindow::RemoveFocisingShaderItem_() const
{
    JAM_ASSERT(GetMainScene().GetFocusingShaderItemOrNull(), "No shader item is currently focused.");

    const Ref<ShaderItem>& pFocusingShaderItem = GetMainScene().GetFocusingShaderItemOrNull();
    GetMainScene().GetProject()->RemoveShader(pFocusingShaderItem);
    GetMainScene().SetFocusingShaderItem(nullptr);
}

void WorkspaceWindow::ClearShaderItems_() const
{

    GetMainScene().GetProject()->ClearShaders();
    GetMainScene().SetFocusingShaderItem(nullptr);
}

void WorkspaceWindow::SetFocusingShaderItem_(
    const Ref<ShaderItem>& _pShaderItem)
{
    if (m_pFocusingShaderItem == _pShaderItem)
    {
        return;
    }

    m_pFocusingShaderItem = _pShaderItem;

    FocusingShaderItemChangedEvent event { _pShaderItem };
    GetApp().DispatchEvent(event);
}
