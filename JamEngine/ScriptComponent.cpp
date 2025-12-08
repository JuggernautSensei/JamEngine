#include "pch.h"

#include "ScriptComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"

namespace jam
{

Json ScriptComponent::Serialize(const Ref<Scene>& _pScene, Entity _entity) const
{
    Json json;
    if (pScript)
    {
        json["script"] = pScript->GetName();
    }
    else
    {
        json["script"] = nullptr;
    }
    return json;
}

bool ScriptComponent::Deserialize(const Json& _json, Ref<Scene>& _pScene, const Entity _entity)
{
    try
    {
        const Json& json = _json.at("script");
        if (json.is_null())
        {
            pScript = nullptr;
        }
        else
        {
            const std::string name = json.get<std::string>();
            pScript                = ScriptDB::GetInstance().Create(name, _entity);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString("ScriptComponent deserialization failed: {}", e.what());
        return false;
    }
}

void ScriptComponent::DrawEditor(Ref<Scene>& _pScene, const Entity _entity)
{
    if (ImGui::BeginTable("ScriptEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // veriable
        const char* preview = pScript ? pScript->GetName().data() : "none";

        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row script
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("script");

        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::BeginCombo("##ScriptComponent", preview, ImGuiComboFlags_NoArrowButton))
        {
            const ScriptDB::ContainerT& container = ScriptDB::GetInstance().GetContainer();
            for (const ScriptInfo& info: std::views::values(container))
            {
                const std::string& name      = info.name;
                const bool         bSelected = pScript && name == pScript->GetName();
                if (ImGui::Selectable(name.c_str(), bSelected))
                {
                    pScript = info.createFn(_entity);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
}

}   // namespace jam