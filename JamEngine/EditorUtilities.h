#pragma once
#include "Entity.h"

namespace jam
{

bool                              SetFileDragDropPayload(const fs::path& _path);
[[nodiscard]] const ImGuiPayload* AcceptFileDragDropPayload();
[[nodiscard]] fs::path            ResolvePathDragDropPayload(const ImGuiPayload* _pPayload);

bool                              SetEntityDragDropPayload(Entity _entity);
[[nodiscard]] const ImGuiPayload* AcceptEntityDragDropPayload();
[[nodiscard]] Entity              ResolveEntityDragDropPayload(const ImGuiPayload* _pPayload);

}   // namespace jam