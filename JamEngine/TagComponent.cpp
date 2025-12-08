#include "pch.h"

#include "TagComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

Json TagComponent::Serialize(const Ref<Scene>& _pScene, Entity _entity) const
{
    Json json;
    json["name"] = name;
    return json;
}

bool TagComponent::Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity)
{
    try
    {
        name = _json["name"].get<std::string>();
        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString(JAM_PRETTY_FUNCTION " failed: {}", e.what());
        return false;
    }
}

void TagComponent::DrawEditor(Ref<Scene>& _pScene, Entity _entity)
{
    if (ImGui::BeginTable("TagEdtior", 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##TagName", &name);

        ImGui::EndTable();
    }
}

}   // namespace jam