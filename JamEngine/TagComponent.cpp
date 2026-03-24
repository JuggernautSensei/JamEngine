#include "pch.h"

#include "TagComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

TagComponent::TagComponent()
{
}

TagComponent::TagComponent(
    const StringView _name)
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
    ArchiveWriter& _writer,
    Scene*,
    Entity) const
{
    _writer.Write(name);
}

void TagComponent::Deserialize(
    ArchiveReader& _reader,
    Scene*,
    Entity)
{
    _reader.Read(name);
}

void TagComponent::DrawEditPanel(
    Scene*,
    Entity)
{
    if (BeginLabelValueTable("TagComponent"))
    {
        ImGui::TableNextColumn();
        Widget_Text("typeName", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##TagName", name);

        ImGui::EndTable();
    }
}

}   // namespace Jam