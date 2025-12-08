#include "pch.h"

#include "CameraComponent.h"

#include "EditorWidgets.h"
#include "JsonUtilities.h"
#include "Scene.h"

namespace jam
{

Json CameraComponent::Serialize(const Ref<Scene>& _pScene, const Entity _entity) const
{
    Json json;
    json["proj"]    = camera.GetProjection();
    json["nearZ"]   = camera.GetNearZ();
    json["farZ"]    = camera.GetFarZ();
    json["fovYRad"] = camera.GetFovYRad();
    json["width"]   = camera.GetWidth();
    json["height"]  = camera.GetHeight();
    json["enable"]  = enable;
    return json;
}

bool CameraComponent::Deserialize(const Json& _json, const Ref<Scene>& _pScene, const Entity _entity)
{
    try
    {
        camera.SetProjection(GetEnumFromJsonOrThrow<eProjection>(_json.at("proj")));
        camera.SetNearZ(_json.at("nearZ").get<float>());
        camera.SetFarZ(_json.at("farZ").get<float>());
        camera.SetFovYRad(_json.at("fovYRad").get<float>());
        camera.SetWidth(_json.at("width").get<float>());
        camera.SetHeight(_json.at("height").get<float>());
        enable = _json.at("enable").get<bool>();
        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString(JAM_PRETTY_FUNCTION " failed: {}", e.what());
        return false;
    }
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
        Widget_FramePaddingAlignedTextUnformatted("projection");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_ProjectionSelector("##CameraProjSelector", proj);

        // near z
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("near / far z");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraNearFarZ", nearFarZ, 0.01f, Camera::k_nearestZ, Camera::k_farthestZ))
        {
            float nearZ = nearFarZ[0];
            float farZ  = nearFarZ[1];
            nearZ       = Clamp(nearZ, Camera::k_nearestZ, farZ - Camera::k_nearFarEpsilon);
            farZ        = Clamp(farZ, nearZ + Camera::k_nearFarEpsilon, Camera::k_farthestZ);
            camera.SetNearZ(nearZ);
            camera.SetFarZ(farZ);
        }

        // fov y
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("fov y (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragAngle("##CameraFovY", &fovYRad, 1.f, 1.f, 179.f))
            camera.SetFovYRad(fovYRad);

        // width / height
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("width / height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraWidthHeight", widthHeight, 0.01f, 0.f, 10000.f))
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
        Widget_FramePaddingAlignedTextUnformatted("enable");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::Checkbox("##CameraEnable", &enable);
        ImGui::EndTable();
    }
}

}   // namespace jam