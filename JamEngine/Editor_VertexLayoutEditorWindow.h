#pragma once
#include "VertexLayout.h"

namespace Jam
{
class VertexLayoutEntry;

class Editor_VertexLayoutEditorWindow
{
public:
    void OnImGuiRender(bool& _pShow);

private:
    void OnClickCreateButton_();
    void OnClickEditButton_() const;
    void OnClickRemoveButton_();

    Ref<VertexLayoutEntry> m_pFocusedVle = nullptr;
};

}   // namespace Jam
