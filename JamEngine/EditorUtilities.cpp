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

constexpr const char* k_editorPayloadTable[] = {
    "EDITOR_FILE",
    "EDITOR_ENTITY",
};

const char* GetEditorPayload(const eEditorPayload _payload)
{
    return k_editorPayloadTable[IntFrom(_payload)];
}

bool SetFileDragDropPayload(const fs::path& _path)
{
    const std::string_view path    = _path.native();
    const char*            strPath = path.data();
    const Int32            len     = NarrowCast<Int32>(path.size()) + 1;
    return ImGui::SetDragDropPayload(GetEditorPayload(eEditorPayload::File), strPath, len);
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

}   // namespace jam