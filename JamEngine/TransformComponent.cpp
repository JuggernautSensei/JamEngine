#include "pch.h"

#include "JsonUtilities.h"
#include "Scene.h"
#include "TransformComponent.h"

#include "EditorWidgets.h"

namespace jam
{

Json TransformComponent::Serialize(const Ref<Scene>& _pScene, Entity _entity) const
{
    const TransformHierarchy& t = _entity.GetTransformHierarchy();
    JAM_ASSERT(t.GetRoot().IsValid(), "root entity is invalid");
    JAM_ASSERT(t.GetOwner().IsValid(), "owner entity is invalid");

    const Transform& local = transformHierarchy.GetLocalTransform();

    Json json;
    json["mtx"] = local.GetMatrix();

    // if not root, save parent UID
    if (!transformHierarchy.IsRoot())
    {
        Entity parent = t.GetParent();
        JAM_ASSERT(parent.IsValid(), "parent entity is invalid");
        json["parent"] = parent.GetUID();
    }
    return json;
}

bool TransformComponent::Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity)
{
    TransformHierarchy& t = _entity.GetTransformHierarchy();
    JAM_ASSERT(t.GetRoot().IsValid(), "root entity is invalid");
    JAM_ASSERT(t.GetOwner().IsValid(), "owner entity is invalid");

    try
    {
        // local transformHierarchy
        const Mat4 mtx = _json.at("mtx").get<Mat4>();
        transformHierarchy.SetLocalMatrix(mtx);

        // root has no parent
        if (!t.IsRoot())
        {
            const UID    uid    = _json.at("parent").get<UID>();
            const Entity parent = _pScene->FindEntity(uid);
            if (parent.IsValid())
            {
                t.SetParent(parent, true);   // keep local transformHierarchy
            }
            else
            {
                SetErrorString("TransformComponent deserialization failed: 'parent' entity with UID {} does not exist.", uid);
                return false;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString(JAM_PRETTY_FUNCTION " failed: {}", e.what());
        return false;
    }
}

void TransformComponent::DrawEditor(Ref<Scene>& _pScene, const Entity _entity)
{
    if (ImGui::BeginTable("TransformEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // coordinate space
        enum class eCoordinate
        {
            World,
            Local
        };
        static eCoordinate s_coord  = eCoordinate::World;
        const bool         bIsWorld = s_coord == eCoordinate::World;

        // get rotation (if not editing)
        static bool s_bEulerEditing = false;
        static Vec3 s_euler         = Vec3::Zero;
        if (s_bEulerEditing == false)
        {
            s_euler = bIsWorld ? transformHierarchy.GetWorldPitchYawRoll() : transformHierarchy.GetLocalPitchYawRoll();
        }

        // get translation & scale
        Vec3 translation = bIsWorld ? transformHierarchy.GetWorldTranslation() : transformHierarchy.GetLocalTranslation();
        Vec3 scale       = bIsWorld ? transformHierarchy.GetWorldScale() : transformHierarchy.GetLocalScale();

        // setup
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // translation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedTextUnformatted("translation");
            ImGui::TableNextColumn();
            if (Widget_DragTranslation("##TransformTranslation", &translation))
            {
                if (bIsWorld)
                {
                    transformHierarchy.SetWorldTranslation(translation);
                }
                else
                {
                    transformHierarchy.SetLocalTranslation(translation);
                }
            }
        }

        // rotation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedTextUnformatted("rotation");
            ImGui::TableNextColumn();
            if (Widget_DragPitchYawRoll("##TransformRotation", &s_euler))
            {
                s_bEulerEditing = true;
                if (bIsWorld)
                {
                    transformHierarchy.SetWorldRotation(s_euler);
                }
                else
                {
                    transformHierarchy.SetWorldRotation(s_euler);
                }
            }

            // editing rotation done
            if (ImGui::IsItemDeactivated())
            {
                s_bEulerEditing = false;
            }
        }

        // scale
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedTextUnformatted("scale");
            ImGui::TableNextColumn();
            if (Widget_DragScale("##TransformScale", &scale, 0.5f, 0.5f))
            {
                if (bIsWorld)
                {
                    transformHierarchy.SetWorldScale(scale);
                }
                else
                {
                    transformHierarchy.SetLocalScale(scale);
                }
            }
        }

        // space radio buttons
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedTextUnformatted("mode");
            ImGui::TableNextColumn();
            ImGui::RadioButton("world", reinterpret_cast<int*>(&s_coord), EnumToInt(eCoordinate::World));
            ImGui::SameLine();
            ImGui::RadioButton("local", reinterpret_cast<int*>(&s_coord), EnumToInt(eCoordinate::Local));
        }

        ImGui::EndTable();
    }
}

}   // namespace jam