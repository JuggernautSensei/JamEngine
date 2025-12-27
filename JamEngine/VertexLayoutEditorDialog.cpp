#include "pch.h"

#include "VertexLayoutEditorDialog.h"

#include "Editor.h"
#include "EditorWidgets.h"
#include "VertexLayoutStorage.h"

namespace jam
{

constexpr const char* k_attribNames[NumEnums<eVertexAttribute>()] = {
    "position",
    "normal",
    "tangent",
    "bitangent",
    "color0",
    "color1",
    "color2",
    "color3",
    "blendIndex",
    "blendWeight",
    "texCoord0",
    "texCoord1",
    "texCoord2",
    "texCoord3",
};

constexpr const char* k_formatNames[NumEnums<eAttributeFormat>()] = {
    "float",
    "float2",
    "float3",
    "float4",
    "unorm8x4",
    "uint8x4",
};

VertexLayout VertexLayoutEditorDialog::BuildLayout_() const
{
    VertexLayout vl;
    for (Int32 i = 0; i < m_count; ++i)
    {
        const Element& elem = m_elems[i];
        vl.Push(elem.attrib, elem.format);
    }
    return vl;
}

VertexLayoutEditorDialog::VertexLayoutEditorDialog(const Ref<VertexLayoutEntry>& _pEntry)
{
    const StringView name = _pEntry->GetName();
    const VertexLayout&    vl   = _pEntry->GetVertexLayout();

    m_oldName = name;
    m_nameBuf = name;
    m_count   = 0;

    for (const VertexLayoutElement& elem: vl.GetElements())
    {
        m_elems[m_count].attrib = elem.attrib;
        m_elems[m_count].format = elem.format;
        ++m_count;
    }

    UpdateState_();
}

bool VertexLayoutEditorDialog::Widget_VertexFormatSelector_(eAttributeFormat& _format)
{
    bool bChanged = false;

    const int   curIdx  = IntFrom(_format);
    const char* preview = k_formatNames[curIdx];
    if (ImGui::BeginCombo("##Format", preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const eAttributeFormat format: EnumRanges<eAttributeFormat>())
        {
            const bool  bSelected = (_format == format);
            const char* name      = k_formatNames[IntFrom(format)];
            if (ImGui::Selectable(name, bSelected))
            {
                if (_format != format)
                {
                    _format = format;
                    UpdateState_();
                    bChanged = true;
                }
            }
        }
        ImGui::EndCombo();
    }
    return bChanged;
}

bool VertexLayoutEditorDialog::Widget_VertexAttributeSelector_(eVertexAttribute& _attrib)
{
    bool bChanged = false;

    const Int32 curIdx  = IntFrom(_attrib);
    const char* preview = k_attribNames[curIdx];
    if (ImGui::BeginCombo("##Attrib", preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const eVertexAttribute attrib: EnumRanges<eVertexAttribute>())
        {
            const bool  bSelected = (_attrib == attrib);
            const char* name      = k_attribNames[IntFrom(attrib)];
            if (ImGui::Selectable(name, bSelected))
            {
                if (_attrib != attrib)
                {
                    _attrib = attrib;
                    UpdateState_();
                    bChanged = true;
                }
            }
        }
        ImGui::EndCombo();
    }
    return bChanged;
}

void VertexLayoutEditorDialog::RemoveElement_(const Int32 _idx)
{
    JAM_ASSERT(_idx >= 0 && _idx < m_count, "invalid element index.");

    for (Int32 j = _idx; j < m_count - 1; ++j)
    {
        m_elems[j] = m_elems[j + 1];
    }

    --m_count;
    UpdateState_();
}

void VertexLayoutEditorDialog::AddNewElement_()
{
    if (m_count >= NumEnums<eVertexAttribute>())
        return;

    Element& elem = m_elems[m_count];
    elem.attrib   = eVertexAttribute::Position;
    elem.format   = eAttributeFormat::Float3;

    ++m_count;
    UpdateState_();
}

void VertexLayoutEditorDialog::operator()()
{
    // name input
    Widget_FrameAlignedText("name");
    ImGui::SameLine();
    SetNextItemWidthToAvailableWidth();
    ImGui::InputText("##EntryName", &m_nameBuf);

    // vertexLayout editor
    if (ImGui::BeginChild("LayoutEditorDialogChild", {}, ImGuiChildFlags_Border))
    {
        if (ImGui::BeginTable("LayoutEditorDialogTable", 5, ImGuiTableFlags_SizingFixedFit))
        {
            // table header
            ImGui::TableSetupColumn("attribute", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("format", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (Int32 i = 0; i < m_count; ++i)
            {
                Element& elem = m_elems[i];

                ImGui::PushID(i);
                ImGui::TableNextRow();

                // attribute column
                ImGui::TableNextColumn();
                SetNextItemWidthToAvailableWidth();
                Widget_VertexAttributeSelector_(elem.attrib);

                // format column
                ImGui::TableNextColumn();
                SetNextItemWidthToAvailableWidth();
                Widget_VertexFormatSelector_(elem.format);

                // size column
                ImGui::TableNextColumn();
                ImGui::Text("%d", ByteFrom(elem.format));

                // offset column
                ImGui::TableNextColumn();
                ImGui::Text("%d", elem.offset);

                // action column
                ImGui::TableNextColumn();
                if (ImGui::Button("remove", CalcFitButtonSize()))
                {
                    RemoveElement_(i);
                    ImGui::PopID();
                    break;   // break loop (element list has changed)
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        if (ImGui::Button("add element", CalcFitButtonSize()))
        {
            AddNewElement_();
        }

    }
    ImGui::EndChild();

    // layout info
    ImGui::Text("element count: %d, stride: %d bytes", m_count, m_stride);

    // action buttons
    const ImVec2 btnRes = CalcFitButtonSize(2);

    // ok
    if (ImGui::Button("ok", btnRes))
    {
        if (OnClickOkButton_())
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    // cancel
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();

    // show popup
    m_popup.Show();
}

void VertexLayoutEditorDialog::UpdateState_()
{
    m_stride              = 0;
    m_bIsAttribDuplicated = true;

    bool usedAttribs[NumEnums<eVertexAttribute>()] = {};

    for (Int32 i = 0; i < m_count; ++i)
    {
        Element&    elem      = m_elems[i];
        const Int32 attribIdx = IntFrom(elem.attrib);

        // calculate offset, stride
        elem.offset = m_stride;
        m_stride += ByteFrom(elem.format);

        // check duplicate attrib
        if (usedAttribs[attribIdx])
        {
            m_bIsAttribDuplicated = false;
        }
        usedAttribs[attribIdx] = true;
    }
}

bool VertexLayoutEditorDialog::OnClickOkButton_()
{
    if (!m_bIsAttribDuplicated)
    {
        m_popup.Open("error", "duplicate vertex attributes are not allowed.");
        return false;
    }

    if (m_nameBuf.empty())
    {
        m_popup.Open("error", "vertex vertexLayout name cannot be empty.");
        return false;
    }

    VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
    if (m_nameBuf != m_oldName && storage.Has(m_nameBuf))
    {
        m_popup.Open("error", "vertex vertexLayout name already exists.");
        return false;
    }

    // apply changes
    const VertexLayout layout = BuildLayout_();
    storage.Remove(m_oldName);
    storage.Add(m_nameBuf, layout);
    return true;
}

}   // namespace jam
