#pragma once
#include "PopupDialog.h"
#include "VertexLayout.h"
#include "VertexLayoutStorage.h"

namespace jam
{

class Editor_VertexLayoutEditor
{
public:
    void OnImGuiRender(bool& _pShow);

    [[nodiscard]] Json OnSerialize() const;
    void               OnDeserialize(const Json& _json) const;

private:
    [[nodiscard]] Json                             SerializeVertexLayoutEntry_(std::string_view _name, const VertexLayout& _layout) const;
    [[nodiscard]] std::optional<VertexLayoutEntry> DeserializeVertexLayoutEntry_(const Json& _json) const;

    void OnClickCreateButton_();
    void OnClickEditButton_() const;
    void OnClickRemoveButton_();

    Ref<VertexLayoutEntry> m_pFocusedEntry = nullptr;
};

}   // namespace jam
