#include "pch.h"

#include "Editor_VertexLayoutEditor.h"

#include "Editor.h"
#include "JsonUtilities.h"
#include "VertexLayout.h"
#include "VertexLayoutEditorDialog.h"

namespace jam
{

void Editor_VertexLayoutEditor::OnImGuiRender(bool& _pShow)
{
    if (!ImGui::Begin("vertex vertexLayout editor", &_pShow))
    {
        ImGui::End();
        return;
    }

    // vertex layout list
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("vertex layout list");

        ImVec2 childWinSize = ImGui::GetContentRegionAvail();
        childWinSize.x *= 0.7f;
        if (ImGui::BeginChild("VertexLayoutListChild", childWinSize, ImGuiChildFlags_Border))
        {
            const VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
            for (const Ref<VertexLayoutEntry>& pEntry: storage.GetContainer() | std::views::values)
            {
                const std::string_view label      = pEntry->GetName();
                const bool             bIsFocused = (m_pFocusedEntry == pEntry);
                if (ImGui::Selectable(label.data(), bIsFocused))
                {
                    m_pFocusedEntry = pEntry;
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    // action buttons
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::NewLine();

        const ImVec2 btnRes = CalcFitButtonSize();
        if (ImGui::Button("create", btnRes))
            OnClickCreateButton_();

        ImGui::BeginDisabled(!m_pFocusedEntry);
        if (ImGui::Button("edit", btnRes))
            OnClickEditButton_();

        if (ImGui::Button("remove", btnRes))
            OnClickRemoveButton_();
        ImGui::EndDisabled();

        if (ImGui::Button("close", btnRes))
            _pShow = false;
    }
    ImGui::EndGroup();
    ImGui::End();
}

Json Editor_VertexLayoutEditor::OnSerialize() const
{
    const VertexLayoutStorage&             storage   = Editor::GetVertexLayoutStorage();
    const VertexLayoutStorage::ContainerT& container = storage.GetContainer();

    Json json;
    for (const Ref<VertexLayoutEntry>& pEntry: container | std::views::values)
    {
        const std::string_view name   = pEntry->GetName();
        const VertexLayout&    layout = pEntry->GetLayout();
        json.push_back(SerializeVertexLayoutEntry_(name, layout));
    }
    return json;
}

void Editor_VertexLayoutEditor::OnDeserialize(const Json& _json) const
{
    // clear existing entries
    VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
    storage.Clear();

    // deserialize entries
    for (const Json& json: _json)
    {
        if (std::optional<VertexLayoutEntry> opt = DeserializeVertexLayoutEntry_(json))
        {
            const VertexLayoutEntry& layout = opt.value();
            storage.Add(layout.GetName(), layout.GetLayout());
        }
    }
}

Json Editor_VertexLayoutEditor::SerializeVertexLayoutEntry_(std::string_view _name, const VertexLayout& _layout) const
{
    Json output;
    output["name"] = _name;

    Json elemsJson;
    for (const VertexLayoutElement& elem: _layout.GetElements())
    {
        Json elemJson;
        elemJson["attrib"] = elem.attrib;
        elemJson["format"] = elem.format;
        elemsJson.emplace_back(std::move(elemJson));
    }
    output["elements"] = std::move(elemsJson);
    return output;
}

std::optional<VertexLayoutEntry> Editor_VertexLayoutEditor::DeserializeVertexLayoutEntry_(const Json& _json) const
{
    try
    {
        const std::string name      = _json.at("name").get<std::string>();
        const Json&       elemsJson = _json.at("elements");

        VertexLayout vl;
        for (const Json& elemJson: elemsJson)
        {
            const eVertexAttribute attrib = GetEnumFromJsonOrThrow<eVertexAttribute>(elemJson.at("attrib"));
            const eAttributeFormat format = GetEnumFromJsonOrThrow<eAttributeFormat>(elemJson.at("format"));
            vl.Add(attrib, format);
        }

        return VertexLayoutEntry { name, vl };
    }
    catch (const std::exception& e)
    {
        Log::Warn(JAM_PRETTY_FUNCTION " failed to deserialize vertex vertexLayout entry: {}", e.what());
        return std::nullopt;
    }
}

void Editor_VertexLayoutEditor::OnClickRemoveButton_()
{
    JAM_ASSERT(m_pFocusedEntry, "No vertex vertexLayout is selected for removal.");
    Editor::GetVertexLayoutStorage().Remove(m_pFocusedEntry->GetName());
    m_pFocusedEntry = nullptr;
}

void Editor_VertexLayoutEditor::OnClickCreateButton_()
{
    VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
    const char*          name    = "new vertexLayout";
    Int32                suffix  = 0;

    while (storage.Has(name))
    {
        name = FormatStringToTmpBuffer("new vertex layout %d", ++suffix);
    }

    VertexLayout vertexLayout;   // dummy vertexLayout
    storage.Add(name, vertexLayout);
    m_pFocusedEntry = storage.Get(name);
}

void Editor_VertexLayoutEditor::OnClickEditButton_() const
{
    JAM_ASSERT(m_pFocusedEntry, "No vertex vertexLayout is selected for editing.");
    Editor::OpenCorePopupDialog("vertex vertexLayout editor", VertexLayoutEditorDialog { m_pFocusedEntry });
}

}   // namespace jam