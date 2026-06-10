#include "pch.h"

#include "PointLightComponent.h"

#include "Camera.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void PointLightComponent::Serialize(
    ArchiveWriter& _writer,
    Scene*,
    Entity) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::Deserialize(
    ArchiveReader& _reader,
    Scene*,
    Entity)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
}

void PointLightComponent::DrawEditPanel(
    Scene*,
    Entity)
{
   /* if (BeginLabelValueTable("PointLightComponent"))
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
        Widget_DragFloat("##PointlightIntensity", intensity, 0.01f, 0.f, Limits<float>::kMax);

        ImGui::TableNextColumn();
        Widget_Text("fall off start", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##PointlightFallOffStart", fallOffStart, 0.01f, 0.f, fallOffEnd, "%.3fm");

        ImGui::TableNextColumn();
        Widget_Text("fall off end", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##PointlightFallOffEnd", fallOffEnd, 0.01f, fallOffStart, Camera::kMaxZ, "%.3fm");

        ImGui::EndTable();
    }*/
}

}   // namespace Jam