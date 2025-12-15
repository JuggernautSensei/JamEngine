#include "pch.h"

#include "CameraComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"
#include "Scene.h"

namespace jam
{

Json CameraComponent::Serialize(const Ref<Scene>& _pScene, const Entity _entity) const
{
    Json json;
    json["camera"] = camera;
    json["enable"] = enable;
    return json;
}

void CameraComponent::Deserialize(const Json& _json, const Ref<Scene>& _pScene, const Entity _entity)
{
    TryParseJsonField(_json, "camera", camera);
    TryParseJsonField(_json, "enable", enable);
}

void CameraComponent::DrawEditor(const Ref<Scene>& _pScene, const Entity _entity)
{
    if (ImGui::BeginTable("CameraEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // variables
        eProjection proj          = camera.GetProjection();
        float       nearFarZ[]    = { camera.GetNearZ(), camera.GetFarZ() };
        float       fovYRad       = camera.GetFovYRad();
        float       widthHeight[] = { camera.GetWidth(), camera.GetHeight() };

        // projection
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("projection");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_ProjectionEnumCombo("##CameraProjSelector", proj);

        // near z
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("near / far z");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraNearFarZ", nearFarZ, 0.01f, k_cameraNearestZ, k_cameraFarestZ))
        {
            float nearZ = nearFarZ[0];
            float farZ  = nearFarZ[1];
            camera.SetNearZ(nearZ);
            camera.SetFarZ(farZ);
        }

        // fov y
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fov y (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragAngle("##CameraFovY", fovYRad, 0.5f, k_cameraMinFovYRad, k_cameraMaxFovYRad))
        {
            camera.SetFovYRad(fovYRad);
        }

        // width / height
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("width / height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraWidthHeight", widthHeight, 0.01f, k_cameraMinWidthHeight, k_cameraMaxWidthHeight))
        {
            float width  = widthHeight[0];
            float height = widthHeight[1];
            width        = Clamp(width, 0.1f, 10000.f);
            height       = Clamp(height, 0.1f, 10000.f);
            camera.SetWidth(width);
            camera.SetHeight(height);
        }

        // main
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("enable");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::Checkbox("##CameraEnable", &enable);
        ImGui::EndTable();
    }
}

}   // namespace jam