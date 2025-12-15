#include "pch.h"

#include "Json.h"
#include "Scene.h"
#include "SceneComponent.h"

#include "EditorWidgets.h"

namespace jam
{

SceneComponent::SceneComponent()
{
}

SceneComponent::SceneComponent(
    const SceneNode& _node)
    : node(_node)
{
}

Json SceneComponent::Serialize(
    const Ref<Scene>& _pScene,
    Entity            _entity) const
{
    const SceneNode& node = _entity.GetSceneNode();
    JAM_ASSERT(node.GetRoot().IsValid(), "root entity is invalid");
    JAM_ASSERT(node.GetOwner().IsValid(), "owner entity is invalid");

    // local matrix
    Json json;
    json["mtx"] = node.GetLocalTransform().GetMatrix();

    // if has local transform, save it
    if (node.HasParent())
    {
        const Entity parent = node.GetParent();
        json["parent"]      = parent.GetUID();
    }

    return json;
}

void SceneComponent::Deserialize(
    const Json&       _json,
    const Ref<Scene>& _pScene,
    Entity            _entity)
{
    // local matrix
    Mat4 mtx;
    TryParseJsonField(_json, "mtx", mtx);
    node.ModifyLocalTransform().SetMatrix(mtx);

    // parent
    if (const auto it = _json.find("parent"); it != _json.end())
    {
        const UID uid = *it;
        if (const Entity parent = _pScene->FindEntity(uid))
        {
            node.SetParent(parent, true);
        }
        else
        {
            JAM_ASSERT(0, "sceneComponent deserialization failed: 'parent' entity with UID {} does not exist.", uid);
            Log::Warn("sceneComponent deserialization failed: 'parent' entity with UID {} does not exist.", uid);
        }
    }
}

void SceneComponent::DrawEditor(Ref<Scene>& _pScene, const Entity _entity)
{
    if (ImGui::BeginTable("TransformEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // coordinate space
        enum class eCoordinate
        {
            World,
            Local
        };

        // static variables
        static eCoordinate s_coord       = eCoordinate::World;
        static bool        s_bPyrEditing = false;
        static Vec3        s_pyr         = Vec3::Zero;

        const bool       bIsWorld  = s_coord == eCoordinate::World;
        const Transform& transform = bIsWorld ? node.GetWorldTransform() : node.GetLocalTransform();

        // get rotation (if not editing)
        if (s_bPyrEditing == false)
        {
            s_pyr = transform.ToPitchYawRoll();
        }

        // get translation & scale
        Vec3 translation = transform.GetTranslation();
        Vec3 scale       = transform.GetScale();

        // setup
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // translation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("translation");
            ImGui::TableNextColumn();
            if (Widget_DragTranslation("##TransformTranslation", &translation))
            {
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetTranslation(translation);
                }
                else
                {
                    node.ModifyLocalTransform().SetTranslation(translation);
                }
            }
        }

        // rotation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("rotation");
            ImGui::TableNextColumn();
            if (Widget_DragPitchYawRoll("##TransformRotation", s_pyr))
            {
                s_bPyrEditing = true;
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetRotation(s_pyr);
                }
                else
                {
                    node.ModifyLocalTransform().SetRotation(s_pyr);
                }
            }

            // editing rotation done
            if (ImGui::IsItemDeactivated())
            {
                s_bPyrEditing = false;
            }
        }

        // scale
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("scale");
            ImGui::TableNextColumn();
            if (Widget_DragScale("##TransformScale", scale, 0.5f, 0.5f))
            {
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetScale(scale);
                }
                else
                {
                    node.ModifyLocalTransform().SetScale(scale);
                }
            }
        }

        // space radio buttons
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("mode");
            ImGui::TableNextColumn();
            Widget_EnumRadioButton("world", s_coord, eCoordinate::World);
            ImGui::SameLine();
            Widget_EnumRadioButton("local", s_coord, eCoordinate::Local);
        }

        ImGui::EndTable();
    }
}

}   // namespace jam