#include "pch.h"

#include "TagComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

TagComponent::TagComponent()
{
}

TagComponent::TagComponent(
    const std::string_view _name)
    : name(_name)
{
}

bool TagComponent::operator==(
    const TagComponent& _other) const
{
    return name == _other.name;
}

bool TagComponent::operator!=(
    const TagComponent& _other) const
{
    return !(*this == _other);
}

bool TagComponent::operator==(
    const std::string_view _name) const
{
    return name == _name;
}

bool TagComponent::operator!=(
    const std::string_view _name) const
{
    return !(*this == _name);
}

Json TagComponent::Serialize(
    const Ref<Scene>& _pScene,
    Entity            _entity) const
{
    return name;
}

void TagComponent::Deserialize(
    const Json&       _json,
    const Ref<Scene>& _pScene,
    Entity            _entity)
{
    TryParseJson(_json, name);
}

void TagComponent::DrawEditor(
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    if (ImGui::BeginTable("TagEdtior", 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##TagName", &name);

        ImGui::EndTable();
    }
}

}   // namespace jam