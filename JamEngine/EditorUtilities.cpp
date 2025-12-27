#include "pch.h"

#include "EditorUtilities.h"

#include "SceneManager.h"

namespace jam
{

enum class eEditorPayload
{
    File = 0,
    Entity,
};

const char* GetEditorPayload(const eEditorPayload _payload)
{
    switch (_payload)
    {
        case eEditorPayload::File:
            return "JAM_EDITOR_PAYLOAD_FILE";
        case eEditorPayload::Entity:
            return "JAM_EDITOR_PAYLOAD_ENTITY";
        default:
            JAM_ASSERT(0, "unknown editor payload");
            return "";
    }
}

bool SetFileDragDropPayload(const fs::path& _path)
{
    const StringView path = _path.native();
    const char*      str  = path.GetPtr();
    const Int32      len  = path.GetLengthWithNullTerminator();
    return ImGui::SetDragDropPayload(GetEditorPayload(eEditorPayload::File), str, len);
}

const ImGuiPayload* AcceptFileDragDropPayload()
{
    return ImGui::AcceptDragDropPayload(GetEditorPayload(eEditorPayload::File));
}

fs::path ResolvePathDragDropPayload(const ImGuiPayload* _pPayload)
{
    JAM_ASSERT(_pPayload, "payload is nullptr");
    JAM_ASSERT(_pPayload->IsDataType(GetEditorPayload(eEditorPayload::File)), "payload type mismatch");
    const char* path = static_cast<const char*>(_pPayload->Data);
    return path;
}

bool SetEntityDragDropPayload(const Entity _entity)
{
    const UID uid = _entity.GetUID();
    return ImGui::SetDragDropPayload(GetEditorPayload(eEditorPayload::Entity), &uid, sizeof(UID));
}

const ImGuiPayload* AcceptEntityDragDropPayload()
{
    return ImGui::AcceptDragDropPayload(GetEditorPayload(eEditorPayload::Entity));
}

Entity ResolveEntityDragDropPayload(const ImGuiPayload* _pPayload)
{
    JAM_ASSERT(_pPayload, "payload is nullptr");
    JAM_ASSERT(_pPayload->IsDataType(GetEditorPayload(eEditorPayload::Entity)), "payload type mismatch");
    JAM_ASSERT(SceneManager::HasActiveScene(), "There is no active scene");

    const UID uid = *static_cast<const UID*>(_pPayload->Data);
    return SceneManager::GetActiveScene()->GetEntity(uid);
}

bool BeginComponentEditTable(const char* _label)
{
    if (ImGui::BeginTable(_label, 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }
    else
    {
        return false;
    }
}

}   // namespace jam