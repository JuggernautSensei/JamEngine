#include "pch.h"
#include "ProjectInspectorWindow.h"

#include "Events.h"
#include "ExportProjectDialog.h"
#include "ShaderApp.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"

void ProjectInspectorWindow::OnImGuiRender()
{
    JAM_ASSERT(m_pProjectCache, "No project is currently loaded.");

    if (!ImGui::Begin("project inspector"))
    {
        ImGui::End();
        return;
    }

#pragma region Project Inspector

    if (ImGui::BeginTable("ProjectInspector", 2))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("project typeName", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_ProjectName_();

        ImGui::TableNextColumn();
        Widget_Text("working directory", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_ReadOnlyInputText("##workingDir", m_pProjectCache->GetWorkingDirectory());

        ImGui::EndTable();
    }

#pragma endregion

#pragma region Shader Item Operator

    ImGui::NewLine();
    Widget_Text("shader items  ", true);   // text and little spacing

    // 5 button of management
    ImGui::SameLine();
    const ImVec2 btnSize = CalcFitButtonSize(3);
    if (ImGui::Button("add", btnSize))
    {
        NewShaderItem_();
    }

    ImGui::SameLine();
    if (ImGui::Button("clear", btnSize))
    {
        m_pProjectCache->ClearShaderItems();
        GetShaderApp().ReleaseFocusingShaderItem();
    }

    ImGui::SameLine();
    if (ImGui::Button("export", btnSize))
    {
        ExportProjectDialog dialog { m_pProjectCache };
        GetShaderApp().PushDialog(MakeScoped<ExportProjectDialog>(dialog));
    }

#pragma endregion

#pragma region Shader Item List

    if (ImGui::BeginTable("##ShaderItemTable", 4, ImGuiTableFlags_BordersOuter, { 0.f, -ImGui::GetFrameHeightWithSpacing() }))
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::TableSetupColumn("Grab", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, (ImGui::GetTextLineHeight() + style.FramePadding.x * 2.f) * 2.f + style.ItemInnerSpacing.x);

        const OrderedStringHashMap<ShaderItem>& shaderItems = m_pProjectCache->GetShaderItems();
        for (const auto& [name, item]: shaderItems)
        {
            ImGui::PushID(name.GetPtr());

            // grab
            ImGui::TableNextColumn();
            Widget_Image(GetShaderApp().GetIcon(eIcon::ReorderGrab), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });

            // select + drag drop hitbox
            ImGui::SameLine();
            const bool bSelected = m_pFocusingShaderItemCache && m_pFocusingShaderItemCache->name == name;
            const bool bClicked  = ImGui::Selectable("##Hitbox", bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0.f, ImGui::GetFrameHeight() });
            if (bClicked || ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                GetShaderApp().SetFocusingShaderItem(name);
            }

            // open popup
            ImGui::OpenPopupOnItemClick("Menu", ImGuiPopupFlags_MouseButtonRight);

            // drag drop source
            if (ImGui::BeginDragDropSource())
            {
                ImGui::TextUnformatted(name.GetPtr());
                SetStringDragDropPayload("SHADER_ITEM_PAYLOAD", name);
                ImGui::EndDragDropSource();
            }

            // darg drop target
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("SHADER_ITEM_PAYLOAD"))
                {
                    const String from = ResolveStringDragDropPayload(pPayload);
                    const String to   = name;
                    app.SubmitCommandToMainThread(Bind([this, from, to] { m_pProjectCache->MoveShaderItemBefore(from, to); }));
                }
                ImGui::EndDragDropTarget();
            }

            // enable checkbox
            ImGui::TableNextColumn();
            bool bEnabled = item.bEnabled;
            if (ImGui::Checkbox("##Enable", &bEnabled))
            {
                m_pProjectCache->SetShaderItemEnabled(name, bEnabled);
            }

            // item typeName
            ImGui::TableNextColumn();
            Widget_Text(name.GetPtr(), true);

            // remove button
            ImGui::TableNextColumn();
            if (Widget_ImageButton("##Remove", GetShaderApp().GetIcon(eIcon::Remove), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
            {
                RemoveShaderItem_(name);
            }

            // clone button
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            if (Widget_ImageButton("##Clone", GetShaderApp().GetIcon(eIcon::Clone), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
            {
                CloneShaderItem_(name);
            }

            // Menu
            if (ImGui::BeginPopup("Menu"))
            {
                if (ImGui::MenuItem("clone"))
                {
                    CloneShaderItem_(name);
                }

                if (ImGui::MenuItem("remove"))
                {
                    RemoveShaderItem_(name);
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // num of shader items
    ImGui::AlignTextToFramePadding();
    ImGui::Text("total: %d", m_pProjectCache->GetShaderItems().GetSize());

#pragma endregion

    ImGui::End();
}

void ProjectInspectorWindow::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher(_event);
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ProjectInspectorWindow::OnProjectChangedEvent_);
    dispatcher.Dispatch<FocusingShaderItemChangedEvent>(this, &ProjectInspectorWindow::OnFocusingShaderItemChagnedEvent_);
}

void ProjectInspectorWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent&)
{
    m_pProjectCache   = GetShaderApp().GetProjectOrNull();
    m_projectNameEdit = m_pProjectCache ? m_pProjectCache->GetName() : StringView {};
}

void ProjectInspectorWindow::OnFocusingShaderItemChagnedEvent_(
    const FocusingShaderItemChangedEvent&)
{
    m_pFocusingShaderItemCache = GetShaderApp().GetFocusingShaderItemOrNull();
}

void ProjectInspectorWindow::Widget_ProjectName_()
{
    Widget_InputText("##ProjectNameEdit", m_projectNameEdit);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_pProjectCache->Rename(m_projectNameEdit);
    }
}

void ProjectInspectorWindow::NewShaderItem_() const
{
    JAM_ASSERT(m_pProjectCache, "No project is currently loaded.");

    ShaderItem item = {};
    item.name       = "new shader item";
    for (Int32 i = 0; m_pProjectCache->ContainsShaderItem(item.name); ++i)
    {
        item.name.Format("new shader item {}", i);
    }

    m_pProjectCache->AddShaderItem(item);
    GetShaderApp().SetFocusingShaderItem(item.name);
}

void ProjectInspectorWindow::CloneShaderItem_(
    const StringView _name) const
{
    JAM_ASSERT(m_pProjectCache, "No project is currently loaded.");

    ShaderItem item = m_pProjectCache->GetShaderItem(_name);
    for (Int32 i = 0; m_pProjectCache->ContainsShaderItem(item.name); ++i)
    {
        item.name.Format("{} {}", _name, i);
    }

    m_pProjectCache->AddShaderItem(item);
    GetShaderApp().SetFocusingShaderItem(item.name);
}

void ProjectInspectorWindow::RemoveShaderItem_(
    const StringView _name) const
{
    JAM_ASSERT(m_pProjectCache, "No project is currently loaded.");

    if (m_pFocusingShaderItemCache && m_pFocusingShaderItemCache->name == _name)
    {
        GetShaderApp().ReleaseFocusingShaderItem();
    }

    const String name = _name;
    app.SubmitCommandToMainThread(Bind([this, name] { m_pProjectCache->RemoveShaderItem(name); }));
}
