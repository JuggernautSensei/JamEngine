#include "pch.h"

#include "ScriptComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"
#include "JsonUtilities.h"

namespace jam
{

static bool Widget_ScriptCombo(const Entity _entity, Ref<Script>& _pScript)
{
    bool bChanged = false;

    const bool       bHasScript = (_pScript != nullptr);
    const StringView preview    = bHasScript ? _pScript->GetTypeName() : "none";

    if (ImGui::BeginCombo("##ScriptComponent", preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        const ScriptDB& db = ScriptDB::GetInstance();
        for (const ScriptInfo& info: db.GetContainer() | std::views::values)
        {
            const StringView label     = info.name;
            const bool       bSelected = bHasScript && (preview == info.name);

            if (ImGui::Selectable(label.GetPtr(), bSelected))
            {
                _pScript = info.createFn(_entity);
                bChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

}   // namespace jam

namespace jam
{

void ScriptComponent::Serialize(
    JsonWriter&       _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    if (pScript)
    {
        _writer.Write(pScript->GetTypeName());
    }
    else
    {
        _writer.Write(nullptr);
    }
}

void ScriptComponent::Deserialize(
    JsonReader&       _reader,
    const Ref<Scene>& _pScene,
    const Entity      _owner)
{
    if (_reader.IsNull())
    {
        pScript = nullptr;
    }
    else
    {
        std::string name;
        if (_reader.Read(name))
        {
            const ScriptDB& db = ScriptDB::GetInstance();
            if (db.IsReflected(name))
            {
                pScript = db.Create(name, _owner);
            }
            else
            {
                GlobalBuffer::SetLastError("ScriptComponent - script '{}' is not reflected in ScriptDB", name);
                Log::Error(GlobalBuffer::GetLastError());
                _reader.MarkFailed();
            }
        }
    }
}

void ScriptComponent::DrawEditor(
    Ref<Scene>&  _pScene,
    const Entity _entity)
{
    if (ImGui::BeginTable("ScriptEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("script");

        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_ScriptCombo(_entity, pScript);

        ImGui::EndTable();
    }
}

}   // namespace jam