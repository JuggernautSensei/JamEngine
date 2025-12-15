#include "pch.h"

#include "ScriptComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"

namespace jam
{

Json ScriptComponent::Serialize(
    const Ref<Scene>& _pScene,
    Entity            _entity) const
{
    if (pScript)
    {
        return pScript->GetTypeName();
    }
    else
    {
        return nullptr;
    }
}

void ScriptComponent::Deserialize(
    const Json&  _json,
    Ref<Scene>&  _pScene,
    const Entity _entity)
{
    if (_json.is_null())
    {
        pScript = nullptr;
    }
    else
    {
        std::string     name = _json.get<std::string>();
        const ScriptDB& db   = ScriptDB::GetInstance();
        if (db.IsReflected(name))
        {
            pScript = db.Create(name, _entity);
        }
        else
        {
            Log::Warn("ScriptComponent::Deserialize: Script '{}' not found in ScriptDB. Setting script to null.", name);
            pScript = nullptr;
        }
    }
}

void ScriptComponent::DrawEditor(
    Ref<Scene>&  _pScene,
    const Entity _entity)
{
    if (ImGui::BeginTable("ScriptEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        const char* preview = pScript ? pScript->GetTypeName().data() : "none";

        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row script
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("script");

        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::BeginCombo("##ScriptComponent", preview, ImGuiComboFlags_NoArrowButton))
        {
            ScriptDB& db = ScriptDB::GetInstance();
            for (const ScriptInfo& info: db.GetContainer() | std::views::values)
            {
                const std::string& label     = info.name;
                const bool         bSelected = pScript && pScript->GetTypeName() == label;
                if (ImGui::Selectable(label.c_str(), bSelected))
                {
                    pScript = info.createFn(_entity);
                }

                if (bSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
}

}   // namespace jam