#include "pch.h"

#include "VertexLayoutEditorDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "MessageDialog.h"
#include "VertexLayoutEntry.h"

namespace Jam
{

VertexLayoutEditorDialog::VertexLayoutEditorDialog(
    const VertexLayoutEditorDialogDesc& _desc)
    : Dialog(_desc.baseDesc)
    , m_pEditVle(_desc.pVle)
    , m_vl(_desc.pVle->GetVertexLayout())
    , m_name(_desc.pVle->GetName().ToString())
{
}

void VertexLayoutEditorDialog::OnShow()
{
    // name input
    {
        Widget_Text("name", true);
        ImGui::SameLine();
        SetNextItemToAvailWidth();
        Widget_InputText("##EntryName", m_name);
    }

    // vertexLayout editor
    ImGui::Spacing();
    if (ImGui::BeginTable("VertexLayoutEditorTable", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders))
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
        for (VertexLayoutElement vle: m_vl.GetElements())   // copy
        {
            ImGui::PushID(UnderlyingFrom(vle.attrib));

            // attribute column
            ImGui::TableNextColumn();
            if (Widget_EnumCombo("##Attrib", vle.attrib))
            {
                m_vl.Modify(vle.attrib, vle.format);
            }

            // format column
            ImGui::TableNextColumn();
            if (Widget_EnumCombo("##Format", vle.format))
            {
                m_vl.Modify(vle.attrib, vle.format);
            }

            // byteWidth column
            {
                ScratchBuffer buf;
                buf.Format("{}", vle.byteWidth);

                ImGui::TableNextColumn();
                Widget_Text(buf.GetPtr(), true);
            }

            // offset column
            {
                ScratchBuffer buf;
                buf.Format("{}", vle.offset);

                ImGui::TableNextColumn();
                Widget_Text(buf.GetPtr(), true);
            }

            // action column
            if (ImGui::Button("x", btnSize))
            {
                OnClickRemoveButton_(vle.attrib);
            }

            ImGui::PopID();
        }

        // add button
        ImGui::Spacing();
        if (ImGui::Button("add", CalcFitButtonSize()))
        {
            OnClickAddButton_();
        }

        ImGui::EndTable();
    }

    // info
    {
        ScratchBuffer buf;
        buf.Format("stride: {} bytes, {} elements", m_vl.GetStride(), m_vl.GetNumElements());

        ImGui::Spacing();
        Widget_Text(buf.GetPtr(), true);
    }

    ImGui::Spacing();
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

void VertexLayoutEditorDialog::OnClickRemoveButton_(
    const eVertexAttribute _attrib)
{
    // remove deferedly to avoid invalidating iterators
    JAM_ASSERT(m_vl.Has(_attrib), "attribute does not exist in the vertex layout.");

    auto remove = [this, _attrib]
    {
        m_vl.Remove(_attrib);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(remove)));
}

void VertexLayoutEditorDialog::OnClickAddButton_()
{
    // find first unused attribute
    for (const eVertexAttribute attrib: EnumRanges<eVertexAttribute>())
    {
        if (!m_vl.Has(attrib))
        {
            // add with default format
            m_vl.Push(attrib, eAttributeFormat::Float);
            return;
        }
    }

    // all attributes are used
    MessageDialogDesc desc = {};
    desc.baseDesc.title    = "error";
    desc.msg               = "all vertex attributes are already used in the vertex layout.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

bool VertexLayoutEditorDialog::OnClickSaveButton_()
{
    if (m_name.IsEmpty())
    {
        MessageDialogDesc desc = {};
        desc.baseDesc.title    = "error";
        desc.msg               = "vertex layout entry name cannot be empty.";
        JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    const bool       bRepalce = (m_pEditVle->GetName() == m_name);
    VertexLayoutMap& map      = JAM_EDITOR.GetVertexLayoutMap();

    // replace existing entry
    if (bRepalce)
    {
        const Ref<VertexLayoutEntry> pVle = MakeRef<VertexLayoutEntry>(m_name, m_vl);
        map.InsertOrReplace(pVle);
        return true;
    }

    // renamed
    if (map.Has(m_name))
    {
        ScratchBuffer buf;
        buf.Format("vertex layout entry named '{}' already exists.", m_name);

        MessageDialogDesc desc = {};
        desc.baseDesc.title    = "error";
        desc.msg               = buf.ToStringView();
        JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    // erase old entry and insert new one
    map.Remove(m_pEditVle->GetName());
    map.Insert(MakeRef<VertexLayoutEntry>(m_name, m_vl));
    return true;
}

}   // namespace Jam
