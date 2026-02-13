#include "pch.h"

#include "PointLightComponent.h"

#include "Camera.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void PointLightComponent::Serialize(
    ArchiveWriter& _writer,
    const Ref<Scene>&,
    Entity) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::Deserialize(
    ArchiveReader& _reader,
    const Ref<Scene>&,
    Entity)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::DrawEditPanel(
    const Ref<Scene>&,
    Entity)
{
    if (BeginLabelTable("PointLightComponent"))
    {
        ImGui::TableNextColumn();
        Widget_Text("radiance", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        ImGui::ColorEdit3("##PointlightRadiance", &radiance.x);

        ImGui::TableNextColumn();
        Widget_Text("intensity", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##PointlightIntensity", intensity, 0.01f, 0.f, Limits<float>::k_max);

        ImGui::TableNextColumn();
        Widget_Text("fall off start", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##PointlightFallOffStart", fallOffStart, 0.01f, 0.f, fallOffEnd, "%.3fm");

        ImGui::TableNextColumn();
        Widget_Text("fall off end", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##PointlightFallOffEnd", fallOffEnd, 0.01f, fallOffStart, Camera::k_maxZ, "%.3fm");

        ImGui::EndTable();
    }
}

}   // namespace Jam