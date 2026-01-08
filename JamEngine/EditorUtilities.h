#pragma once
#include "Entity.h"
#include "IAsset.h"
#include "Magics.h"
#include "WindowsUtilities.h"

namespace jam
{

struct AttributeFormatInfo
{
    StringView editorDisplayName = {};
    Int32      byteWidth         = 0;
};

// file dialog filters
constexpr FileDialogFilter k_jamSceneFdFilter = { "jam engine scene", k_jamSceneExtFilter };

bool                              SetFileDragDropPayload(const fs::path& _path);
[[nodiscard]] const ImGuiPayload* AcceptFileDragDropPayload();
[[nodiscard]] fs::path            ResolvePathDragDropPayload(const ImGuiPayload* _pPayload);

bool                              SetEntityDragDropPayload(Entity _entity);
[[nodiscard]] const ImGuiPayload* AcceptEntityDragDropPayload();
[[nodiscard]] Entity              ResolveEntityDragDropPayload(const ImGuiPayload* _pPayload);

// table layout
// | label | ui |
// ImGui::TableNextColumn() -> render label -> ImGui::TableNextColumn() -> render ui -> repeat...
bool BeginLabelValueTable(const char* _label);

// misc
[[nodiscard]] StringView EditorDisplayStringFrom(ImGuiKey _key);
[[nodiscard]] StringView EditorDisplayStringFrom(eAssetType _type);
[[nodiscard]] StringView EditorDisplayStringFrom(eVertexAttribute _attrib);

[[nodiscard]] AttributeFormatInfo AttributeFormatInfoFrom(eAttributeFormat _format);

[[nodiscard]] ImVec2 GetContentRegionAvailScaled(float _sx, float _sy);

}   // namespace jam