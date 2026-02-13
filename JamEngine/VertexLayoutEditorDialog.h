#pragma once
#include "Dialog.h"

namespace Jam
{

class VertexLayoutEntry;

struct VertexLayoutEditorDialogDesc
{
    DialogDesc             baseDesc = {};
    Ref<VertexLayoutEntry> pVle     = nullptr;
};

class VertexLayoutEditorDialog : public Dialog
{
public:
    explicit VertexLayoutEditorDialog(const VertexLayoutEditorDialogDesc& _desc);
    void OnShow() override;

private:
    void OnClickRemoveButton_(eVertexAttribute _attrib);
    void OnClickAddButton_();
    bool OnClickSaveButton_();

    Ref<VertexLayoutEntry> m_pEditVle = nullptr;
    VertexLayout           m_vl       = {};
    String                 m_name     = {};
};

}   // namespace Jam
