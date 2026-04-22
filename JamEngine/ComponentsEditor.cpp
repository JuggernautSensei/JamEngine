#include "pch.h"

#include "Application.h"

#include "Components.h"
#include "EditorUtilities.h"
#include "ModelAsset.h"
#include "SceneLayer.h"
#include "ScriptMetaManager.h"
#include "Textures.h"

namespace
{

constexpr float kLabelWidth = 100.f;   // 라벨 컬럼의 너비

}   // namespace

namespace jam
{

void TagComponent::DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity)
{
    if (ImGui::BeginTable("tag component", 2))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelWidth);

        // 1행
        {
            ImGui::TableNextRow();

            // 라벨 1열
            {
                ImGui::TableNextColumn();
                ImGui::Text("tag");
            }

            // 입력 2열
            {
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##tag", &name);
            }
        }

        ImGui::EndTable();
    }
}

void TransformComponent::DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity)
{
    if (ImGui::BeginTable("transform component", 2))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelWidth);

        // 위치 1행
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("translate");

            ImGui::TableNextColumn();
            editor::DragTranslate("##translate", reinterpret_cast<float*>(&transform.translate));
        }

        // 회전 2행
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("rotate");

            ImGui::TableNextColumn();

            // quaternion editor는 엔티티 마다 다른 라벨을 유지해야함
            std::string label = std::format("##rot{}", _ownerEntity.GetID());   // 높은 확률로 SSO 최적화
            editor::DragQuaternion(label.c_str(), reinterpret_cast<float*>(&transform.rotate));
        }

        // 스케일 3행
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("scale");

            ImGui::TableNextColumn();
            editor::DragScale3("##scale", reinterpret_cast<float*>(&transform.scale));
        }

        ImGui::EndTable();
    }
}



}   // namespace jam