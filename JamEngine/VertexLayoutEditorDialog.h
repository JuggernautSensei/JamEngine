#pragma once
#include "Dialog.h"
#include "OrderedHashMap.h"

namespace Jam
{

class VertexLayoutEntry;

struct VertexLayoutEditorDialogInfo
{
    StringView             title = {};
    ImVec2                 size  = {};
    Ref<VertexLayoutEntry> pVle  = nullptr;
};

class VertexLayoutEditorDialog : public Dialog
{
public:
    explicit VertexLayoutEditorDialog(const VertexLayoutEditorDialogInfo& _info);
    void OnShow() override;

private:
    bool OnClickSaveButton_() const;

    void PushNodeDefered_(eVertexAttribute _attrib, eAttributeFormat _format);
    void RemoveNodeDefered_(eVertexAttribute _attrib);
    void ModifyNodeDefered_(eVertexAttribute _attrib, eAttributeFormat _format);
    bool IsElementAllUsed() const;

    bool Widget_VertexAttributeCombo_(const char* _label, eVertexAttribute& _attrib) const;
    bool Widget_AddElementButton_(const char* _label, ImVec2 _size);

    String                                             m_oldName = {};
    String                                             m_newName = {};
    OrderedHashMap<eVertexAttribute, eAttributeFormat> m_elems   = {};
};

}   // namespace Jam
