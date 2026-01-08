#include "pch.h"

#include "ScriptComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"
#include "JsonUtilities.h"

namespace jam
{

void ScriptComponent::Serialize(
    JsonWriter        _writer,
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
    JsonReader        _reader,
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

void ScriptComponent::Panel_EditComponent(
    Ref<Scene>&  _pScene,
    const Entity _entity)
{
    if (BeginLabelValueTable("ScriptComponent"))
    {
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("script");

        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_ScriptCombo("##ScriptCombo", pScript, _entity);

        ImGui::EndTable()();
    }
}

}   // namespace jam