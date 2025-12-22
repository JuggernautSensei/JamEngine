#include "pch.h"

#include "PointLightComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void PointLightComponent::Serialize(
    JsonWriter&       _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::Deserialize(
    JsonReader&       _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::DrawEditor(Ref<Scene>& _pScene, Entity _entity)
{
    if (ImGui::BeginTable("PointLightEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radiance");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::ColorEdit3("##PointlightRadiance", &radiance.x);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("intensity");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightIntensity", &intensity, 0.1f, 0.f, 10000.f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off start");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightFallOffStart", &fallOffStart, 0.1f, 0.f, fallOffEnd - k_epsilon);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off end");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightFallOffEnd", &fallOffEnd, 0.1f, fallOffStart + k_epsilon, 10000.f);

        ImGui::EndTable();
    }
}

}   // namespace jam