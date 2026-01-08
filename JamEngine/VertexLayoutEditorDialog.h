#pragma once
#include "Dialog.h"

namespace jam
{

class VertexLayoutEntry;

class VertexLayoutEditorDialog : public Dialog
{
    enum class eDirection
    {
        Up,
        Down,
    };

    constexpr Int32 k_invalidIndex = -1;

public:
    explicit VertexLayoutEditorDialog(const Ref<VertexLayoutEntry>& _pVle);
    void OnShow() override;

private:
    bool Widget_VertexAttributeCombo_(const char* _label, eVertexAttribute& _attrib) const;
    bool Widget_AttributeFormatCombo_(const char* _label, eAttributeFormat& _format) const;

    void OnClickRemoveButton_(eVertexAttribute _attrib);
    void OnClickAddButton_();
    bool OnClickSaveButton_();

    VertexLayout m_vl = {};

    std::string m_name    = {};
    std::string m_oldName = {};
};

}   // namespace jam
