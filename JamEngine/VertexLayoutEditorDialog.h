#pragma once
#include "PopupDialog.h"
#include "VertexLayout.h"
#include "VertexLayoutStorage.h"

namespace jam
{

class VertexLayoutEditorDialog
{
    struct Element
    {
        eVertexAttribute attrib = eVertexAttribute::Unknown;
        eAttributeFormat format = eAttributeFormat::Unknown;
        Int32            offset = 0;
    };

public:
    explicit VertexLayoutEditorDialog(const Ref<VertexLayoutEntry>& _pEntry);
    void operator()();

private:
    bool OnClickOkButton_();

    bool Widget_VertexFormatSelector_(eAttributeFormat& _format);
    bool Widget_VertexAttributeSelector_(eVertexAttribute& _attrib);

    void RemoveElement_(Int32 _idx);
    void AddNewElement_();
    void UpdateState_();

    [[nodiscard]] VertexLayout BuildLayout_() const;

    // buffer
    std::string m_oldName;
    std::string m_nameBuf;

    // vertex vertexLayout
    Element m_elems[EnumCount<eVertexAttribute>()];
    Int32   m_count  = 0;
    Int32   m_stride = 0;
    bool    m_bIsAttribDuplicated = false;

    // popup
    PopupDialog m_popup;
};

}   // namespace jam
