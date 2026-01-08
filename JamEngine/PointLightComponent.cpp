#include "pch.h"

#include "PointLightComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void PointLightComponent::Serialize(
    JsonWriter        _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::Deserialize(
    JsonReader        _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::Panel_EditComponent(Ref<Scene>& _pScene, Entity _entity)
{
    // if (ImGui::BeginTable("PointLightEditor", 2, ImGuiTableFlags_SizingFixedFit))
    if (BeginLabelValueTable("PointLightComponent"))
    {
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radiance");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        ImGui::ColorEdit3("##PointlightRadiance", &radiance.x);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("intensity");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##PointlightIntensity", &intensity, 0.01f, 0.f);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off start");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##PointlightFallOffStart", &fallOffStart, 0.01f, 0.f, fallOffEnd, "%.2fm");

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off end");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##PointlightFallOffEnd", &fallOffEnd, 0.01f, fallOffStart, Camera::k_maxZ, "%.2fm");

        ImGui::EndTable()();
    }
}

}   // namespace jam