#include "pch.h"

#include "VertexLayoutEditorDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "VertexLayoutEntry.h"

namespace jam
{

VertexLayoutEditorDialog::VertexLayoutEditorDialog(
    const Ref<VertexLayoutEntry>& _pVle)
    : Dialog("vertex layout editor")
    , m_vl(_pVle->GetVertexLayout())
    , m_name(_pVle->GetName().ToString())
    , m_oldName(m_name)
{
}

void VertexLayoutEditorDialog::OnShow()
{
    // name input
    {
        Widget_FrameAlignedText("name");
        ImGui::SameLine();
        SetNextItemAvailWidth();
        ImGui::InputText("##EntryName", &m_name);
    }

    // vertexLayout editor
    ImGui::Spacing();
    if (ImGui::BeginTable("VertexLayoutEditorTable", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders))
    {
        const ImVec2 btnSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
        const float  btnW    = btnSize.x + ImGui::GetStyle().FramePadding.x * 2.f;

        // table header
        ImGui::TableSetupColumn("attribute", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("format", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("byteWidth", ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, btnW);
        ImGui::TableHeadersRow();

        // render elements
        for (VertexLayoutElement vle: m_vl.GetElements())
        {
            ImGui::PushID(IntFrom(vle.attrib));

            // attribute column
            {
                ImGui::TableNextColumn();
                if (Widget_VertexAttributeCombo_("##Attrib", vle.attrib))
                {
                    m_vl.Modify(vle.attrib, vle.format);
                }
            }

            // format column
            {
                ImGui::TableNextColumn();
                if (Widget_AttributeFormatCombo_("##Format", vle.format))
                {
                    m_vl.Modify(vle.attrib, vle.format);
                }
            }

            // byteWidth column
            {
                const StringView text = GlobalBuffer::FormatTmp("{}", vle.byteWidth);
                ImGui::TableNextColumn();
                Widget_FrameAlignedText(text.GetPtr());
            }

            // offset column
            {
                const StringView text = GlobalBuffer::FormatTmp("{}", vle.byteWidth);
                ImGui::TableNextColumn();
                Widget_FrameAlignedText(text.GetPtr());
            }

            // action column
            if (ImGui::Button("x", btnSize))
            {
                OnClickRemoveButton_(vle.attrib);
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // add
    {
        ImGui::Spacing();
        if (ImGui::Button("add", CalcFitButtonSize()))
        {
            OnClickAddButton_();
        }
    }

    // info
    {
        ImGui::Spacing();
        const StringView info = GlobalBuffer::FormatTmp("stride: {} bytes, {} elements", m_vl.GetStride(), m_vl.GetNumElements());
        Widget_FrameAlignedText(info.GetPtr());
    }

    ImGui::Spacing();
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);
        if (ImGui::Button("save", btnSize))
        {
            if (OnClickSaveButton_())
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("close", btnSize))
        {
            ImGui::CloseCurrentPopup();
        }
    }
}

void VertexLayoutEditorDialog::OnClickRemoveButton_(
    const eVertexAttribute _attrib)
{
    JAM_ASSERT(m_vl.Has(_attrib), "attribute does not exist in the vertex layout.");
    // remove deferedly to avoid invalidating iterators

    VertexLayout* pVl = &m_vl;   // for passing to lambda
    Application&  app = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_METHOD(pVl, VertexLayout::Remove, _attrib));
}

bool VertexLayoutEditorDialog::Widget_VertexAttributeCombo_(
    const char*       _label,
    eVertexAttribute& _attrib) const
{
    bool bChanged = false;

    const StringView preview = EditorDisplayStringFrom(_attrib);
    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const eVertexAttribute attrib: eVertexAttribute {})
        {
            // if attrib is already used in the layout, skip it
            if (m_vl.Has(attrib))
            {
                continue;
            }

            const StringView label = EditorDisplayStringFrom(attrib);
            if (ImGui::Selectable(label.GetPtr()))
            {
                _attrib  = attrib;
                bChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    return bChanged;
}

bool VertexLayoutEditorDialog::Widget_AttributeFormatCombo_(
    const char*       _label,
    eAttributeFormat& _format) const
{
    bool                      bChanged    = false;
    const AttributeFormatInfo previewInfo = AttributeFormatInfoFrom(_format);
    const StringView          preview     = previewInfo.editorDisplayName;

    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const eAttributeFormat format: eAttributeFormat {})
        {
            const bool                bSelected = (_format == format);
            const AttributeFormatInfo info      = AttributeFormatInfoFrom(format);
            const StringView          label     = info.editorDisplayName;

            if (ImGui::Selectable(label.GetPtr(), bSelected))
            {
                _format  = format;
                bChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

void VertexLayoutEditorDialog::OnClickAddButton_()
{
    // find first unused attribute
    for (const eVertexAttribute attrib: eVertexAttribute {})
    {
        if (!m_vl.Has(attrib))
        {
            // add with default format
            m_vl.Push(attrib, eAttributeFormat::Float);
            return;
        }
    }

    // all attributes are used
    Editor::PushMessageDialog("error", "all vertex attributes are already used in the vertex layout.");
}

bool VertexLayoutEditorDialog::OnClickSaveButton_()
{
    if (m_name.empty())
    {
        Editor::PushMessageDialog("error", "vertex layout name cannot be empty.");
        return false;
    }

    VertexLayoutMap& map = Editor::GetVertexLayoutMap();
    if (m_name != m_oldName && map.contains(m_name))
    {
        Editor::PushMessageDialog("error", "vertex layout name already exists.");
        return false;
    }

    // apply changes
    map.erase(m_oldName);
    map.emplace(m_name, MakeRef<VertexLayoutEntry>(m_name, m_vl));
    return true;
}

}   // namespace jam
