#include "pch.h"

#include "VertexLayoutEditorDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "MessageDialog.h"
#include "VertexLayoutEntryManager.h"
#include "StaticString.h"


namespace Jam
{

VertexLayoutEditorDialog::VertexLayoutEditorDialog(
    const VertexLayoutEditorDialogInfo& _info)
    : Dialog(_info.title, _info.size)
    , m_oldName(_info.pVle->GetName())
    , m_newName(_info.pVle->GetName())
{
    for (const VertexLayoutElement& elem: _info.pVle->GetVertexLayout().GetElems())
    {
        if (m_elems.PushBack(elem.attrib, elem.format).bSucceeded == false)
        {
            JAM_ASSERT(false, "Duplicate vertex attribute in the vertex layout entry being edited.");
        }
    }
}

void VertexLayoutEditorDialog::OnShow()
{
    // name input
    {
        Widget_Text("name", true);
        ImGui::SameLine();
        SetNextItemToAvailWidth();
        Widget_InputText("##EntryName", m_newName);
    }

    // vertexLayout editor
    Int32 offset = 0;   // spec
    Int32 stride = 0;   // spec

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
        for (const auto node: m_elems)
        {
            eVertexAttribute attrib = node.key;
            eAttributeFormat format = node.value;

            ImGui::PushID(AsUnderlying(attrib));

            // calculate byte width and offset
            offset = stride;
            stride += static_cast<Int32>(ByteWidthOf(format));

            // attribute column
            ImGui::TableNextColumn();
            if (Widget_VertexAttributeCombo_("##Attrib", attrib))
            {
                PushNodeDeferred_(attrib, format);
            }

            // format column
            ImGui::TableNextColumn();
            if (Widget_EnumCombo("##Format", format))
            {
                ModifyNodeDeferred_(attrib, format);
            }

            // byteWidth column
            {
                StaticString<64> buf;
                buf.Format("{}", ByteWidthOf(format));

                ImGui::TableNextColumn();
                Widget_Text(buf.GetPtr(), true);
            }

            // offset column
            {
                StaticString<64> buf;
                buf.Format("{}", offset);

                ImGui::TableNextColumn();
                Widget_Text(buf.GetPtr(), true);
            }

            // action column
            if (ImGui::Button("remove", btnSize))
            {
                RemoveNodeDeferred_(attrib);
            }

            ImGui::PopID();
        }

        // add button
        ImGui::Spacing();
        if (Widget_AddElementButton_("add element", btnSize))
        {
        }

        ImGui::EndTable();
    }

    // info
    {
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("stride: %d bytes, %d elements", stride, m_elems.GetSize());
    }

    // save / close buttons
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        // save
        ImGui::Spacing();
        if (ImGui::Button("save", btnSize))
        {
            if (OnClickSaveButton_())
            {
                ImGui::CloseCurrentPopup();
            }
        }

        // close
        ImGui::SameLine();
        if (ImGui::Button("close", btnSize))
        {
            ImGui::CloseCurrentPopup();
        }
    }
}

bool VertexLayoutEditorDialog::OnClickSaveButton_() const
{
    if (m_newName.IsEmpty())
    {
        GetEditor().PushMessageDialog("error", "entry name cannot be empty.");
        return false;
    }

    const bool bNameChanged = (m_newName != m_oldName);
    if (bNameChanged && GetEditor().GetVertexLayoutManager().Contains(m_newName))
    {
        GetEditor().PushMessageDialog("error", "entry name already exists. please choose a different name.");
        return false;
    }

    if (m_elems.IsEmpty())
    {
        GetEditor().PushMessageDialog("error", "vertex layout entry must contain at least one element.");
        return false;
    }

    if (bNameChanged)
    {
        GetEditor().GetVertexLayoutManager().Rename(m_oldName, m_newName);
    }

    // build vertex layout
    VertexLayout vl = {};
    for (const auto node: m_elems)
    {
        vl.Add(node.key, node.value);
    }

    // update
    Ref<VertexLayoutEntry> pEntry = GetEditor().GetVertexLayoutManager().Get(m_oldName);
    pEntry->SetVertexLayout(vl);
    return true;
}

void VertexLayoutEditorDialog::PushNodeDeferred_(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    Application& app = GetApp();
    app.SubmitCommandToMainThread(Bind(
        [this, _attrib, _format]
        {
            m_elems.PushBack(_attrib, _format);
        }));
}

void VertexLayoutEditorDialog::RemoveNodeDeferred_(
    const eVertexAttribute _attrib)
{
    JAM_ASSERT(m_elems.Contains(_attrib), "attribute does not exist in the vertex layout.");

    Application& app = GetApp();
    app.SubmitCommandToMainThread(Bind(
        [this, _attrib]
        {
            m_elems.Remove(_attrib);
        }));
}

void VertexLayoutEditorDialog::ModifyNodeDeferred_(
    const eVertexAttribute _attrib,
    const eAttributeFormat _format)
{
    JAM_ASSERT(m_elems.Contains(_attrib), "attribute does not exist in the vertex layout.");

    Application& app = GetApp();
    app.SubmitCommandToMainThread(Bind(
        [this, _attrib, _format]
        {
            m_elems.Get(_attrib) = _format;
        }));
}

bool VertexLayoutEditorDialog::IsElementAllUsed() const
{
    return m_elems.GetSize() == CountOf<eVertexAttribute>();
}

bool VertexLayoutEditorDialog::Widget_VertexAttributeCombo_(
    const char*       _label,
    eVertexAttribute& _attrib) const
{
    const char* preview = NameOf(_attrib).GetPtr();

    bool bChanged = false;
    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const eVertexAttribute attrib: RangesOf<eVertexAttribute>())
        {
            const bool bCannotSelect = !m_elems.Contains(attrib);

            if (bCannotSelect)
            {
                ImGui::BeginDisabled(true);
            }

            const char* label     = NameOf(attrib).GetPtr();
            const bool  bSelected = (attrib == _attrib);
            if (ImGui::Selectable(label, bSelected))
            {
                bChanged = (attrib != _attrib);
                _attrib  = attrib;
            }

            if (bCannotSelect)
            {
                ImGui::EndDisabled();
            }
        }
        ImGui::EndCombo();
    }

    return bChanged;
}

bool VertexLayoutEditorDialog::Widget_AddElementButton_(
    const char*  _label,
    const ImVec2 _size)
{
    const bool bFull = IsElementAllUsed();
    ImGui::BeginDisabled(bFull);
    if (ImGui::Button(_label, _size))
    {
        for (const eVertexAttribute attrib: RangesOf<eVertexAttribute>())
        {
            if (!m_elems.Contains(attrib))
            {
                PushNodeDeferred_(attrib, eAttributeFormat::Float);
                return true;
            }
        }

        JAM_UNREACHABLE;
    }
    ImGui::EndDisabled();

    if (bFull)
    {
        ImGui::SetItemTooltip("all vertex attributes have been added. remove existing attributes to add new ones.");
    }

    return false;
}

}   // namespace Jam
