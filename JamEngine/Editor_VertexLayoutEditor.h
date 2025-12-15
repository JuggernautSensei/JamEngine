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

private:
    void OnClickCreateButton_();
    void OnClickEditButton_() const;
    void OnClickRemoveButton_();

    Ref<VertexLayoutEntry> m_pFocusedVle = nullptr;
};

}   // namespace jam
