#include "pch.h"

#include "TagComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

TagComponent::TagComponent()
{
}

TagComponent::TagComponent(
    const StringView _name)
    : name(_name.ToString())
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
    const StringView _name) const
{
    return name == _name;
}

bool TagComponent::operator!=(
    const StringView _name) const
{
    return !(*this == _name);
}

void TagComponent::Serialize(
    JsonWriter&       _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    _writer.Write(name);
}

void TagComponent::Deserialize(
    JsonReader&       _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    _reader.Read(name);
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