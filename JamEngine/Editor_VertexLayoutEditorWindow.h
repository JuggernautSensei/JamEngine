#pragma once

namespace Jam
{

class VertexLayoutEntry;

class Editor_VertexLayoutEditorWindow
{
public:
    void OnImGuiRender(bool& _pShow);

private:
    void OnClickNewButton_();
    void OnClickEditButton_(const Ref<VertexLayoutEntry>& _pEntry) const;
    void OnClickRemoveButton_(const Ref<VertexLayoutEntry>& _pEntry);
    void OnClickCloneButton_(const Ref<VertexLayoutEntry>& _pEntry) const;

    void Widget_VertexLayoutEntryListItem_(const Ref<VertexLayoutEntry>& _pVle);

    Ref<VertexLayoutEntry> m_pFocusedVle = nullptr;
};

}   // namespace Jam
