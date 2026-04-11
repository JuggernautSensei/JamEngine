#include "pch.h"
#include "ShaderItemEditorWindow.h"
#include "Events.h"
#include "GUIAssetManager.h"
#include "ShaderApp.h"
#include "../JamEngine/DirIter.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/FileDialog.h"
#include "../JamEngine/STLAlgorithm.h"

void ShaderItemEditorWindow::OnImGuiRender()
{
    if (!ImGui::Begin("shader item editor"))
    {
        ImGui::End();
        return;
    }

    if (!m_pFocusingShaderItemCache)
    {
        Widget_TextFrame("no shader item selected", ImGui::GetContentRegionAvail());
        ImGui::End();
        return;
    }

#pragma region Shader Item Specififcation Editor

    ImGui::SeparatorText("specification");
    if (ImGui::BeginTable("Specification", 2))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("typeName", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EditShaderItemName();

        ImGui::TableNextColumn();
        Widget_Text("shader source", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EditShaderItemShaderSource();

        ImGui::TableNextColumn();
        Widget_Text("entry point", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EditShaderItemEntryPoint();

        ImGui::TableNextColumn();
        Widget_Text("stage", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EditShaderItemStage();

        ImGui::TableNextColumn();
        Widget_Text("compile flags", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EditShaderItemCompileFlags();

        ImGui::EndTable();
    }

#pragma endregion

#pragma region Predefined Macros Editor

    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SeparatorText("pre-defined macros");
    if (ImGui::BeginChild("##ShaderItemMacrosChild", { 0.f, ImGui::GetFrameHeight() * 5.f }, ImGuiChildFlags_ResizeY))
    {
        if (ImGui::BeginTable("##EditSHaderItemMacrosTable", 5, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Reordering", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
            ImGui::TableSetupColumn("##Enabled", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
            ImGui::TableSetupColumn("typeName", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##Actions", ImGuiTableColumnFlags_WidthFixed, (ImGui::GetTextLineHeight() + style.FramePadding.x * 2) * 2 + style.ItemInnerSpacing.x);
            ImGui::TableHeadersRow();

            Int32 idx = 0;
            for (auto it = m_focusingShaderItemMacrosEdit.begin(); it != m_focusingShaderItemMacrosEdit.end(); ++it, ++idx)
            {
                String& name     = it->name;
                String& value    = it->value;
                bool&   bEnabled = it->bEnabled;

                // push for unique hash
                ImGui::PushID(idx);

                // reordering handle
                ImGui::TableNextColumn();
                Widget_Image(GetShaderApp().GetIcon(eIcon::ReorderGrab), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });

                // drag drop for macro reordering
                ImGui::SameLine();
                ImGui::Selectable("##Hitbox", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0.f, ImGui::GetFrameHeight() });
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::TextUnformatted(name.GetPtr());
                    ImGui::SetDragDropPayload("SHADER_MACRO_PAYLOAD", &idx, sizeof(idx));
                    ImGui::EndDragDropSource();
                }

                // drag drop target
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("SHADER_MACRO_PAYLOAD"))
                    {
                        const Int32 srcIdx = *static_cast<const Int32*>(pPayload->Data);
                        if (srcIdx != idx)
                        {
                            GetApp().SubmitCommandToMainThread(Bind(
                                [this, srcIdx, idx] {
                                    VectorSplice(m_focusingShaderItemMacrosEdit, idx, srcIdx);
                                    m_pProjectCache->MoveShaderItemMacroBefore(m_pFocusingShaderItemCache->name, srcIdx, idx);
                                }));
                        }
                    }

                    ImGui::EndDragDropTarget();
                }

                // check box for macro enabled
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##MacroEnabledEdit", &bEnabled))
                {
                    m_pProjectCache->SetShaderItemMacro(m_pFocusingShaderItemCache->name, idx, *it);
                }

                // Macro typeName
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                Widget_InputText("##MacroNameEdit", name);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    m_pProjectCache->SetShaderItemMacro(m_pFocusingShaderItemCache->name, idx, *it);
                }

                // Macros Value
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                Widget_InputText("##MacroValueEdit", value);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    m_pProjectCache->SetShaderItemMacro(m_pFocusingShaderItemCache->name, idx, *it);
                }

                // Macro clone button
                ImGui::TableNextColumn();
                if (Widget_ImageButton("Clone", GetShaderApp().GetIcon(eIcon::Clone), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
                {
                    GetApp().SubmitCommandToMainThread(Bind(
                        [this, idx] {
                            const ShaderItem::Macro newMacro = m_focusingShaderItemMacrosEdit[idx];
                            m_focusingShaderItemMacrosEdit.emplace_back(newMacro);
                            m_pProjectCache->PushShaderItemMacro(m_pFocusingShaderItemCache->name, newMacro);
                        }));
                }

                // Macro remove button
                ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
                if (Widget_ImageButton("Remove", GetShaderApp().GetIcon(eIcon::Remove), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
                {
                    GetApp().SubmitCommandToMainThread(Bind([this, idx] {
                        VectorErase(m_focusingShaderItemMacrosEdit, idx);
                        m_pProjectCache->RemoveShaderItemMacro(m_pFocusingShaderItemCache->name, idx);
                    }));
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    // add new macro button
    if (ImGui::Button("add new macro", CalcFitButtonSize()))
    {
        m_focusingShaderItemMacrosEdit.push_back({});
        m_pProjectCache->PushShaderItemMacro(m_pFocusingShaderItemCache->name, {});
    }

#pragma endregion

#pragma region Shader Source Viewer

    ImGui::SeparatorText("shader source viewer");
    if (ImGui::BeginChild("ShaderSourceViewer", { 0.f, -ImGui::GetFrameHeightWithSpacing() }, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoNavInputs))
    {
        if (m_pFocusingShaderItemCache->pShaderSource)
        {
            const StringView source = m_pFocusingShaderItemCache->pShaderSource->GetSource();
            Widget_ReadOnlyInputTextMultiline("##ShaderSourceTextEdit", source, ImGui::GetContentRegionAvail());
        }
        else
        {
            Widget_TextFrame("no shader source", ImGui::GetContentRegionAvail());
        }
    }
    ImGui::EndChild();

    // buttons
    const ImVec2             btnSize       = CalcFitButtonSize(2);
    const Ref<ShaderSource>& pShaderSource = m_pFocusingShaderItemCache->pShaderSource;

    // open as external editor
    ImGui::BeginDisabled(!pShaderSource);
    if (ImGui::Button("open as", btnSize))
    {
        OpenShaderSourceAsExternalEditor_(pShaderSource->GetPath());
    }

    // choose open external editor
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("system default", 0, m_sourceEditorPathOrEmpty.IsEmpty()))
        {
            m_sourceEditorPathOrEmpty.Clear();
        }

        if (ImGui::MenuItem("choose manually", 0, !m_sourceEditorPathOrEmpty.IsEmpty()))
        {
            FileDialogFilter k_filters[] = {
                { "Executable Files", "*.exe" },
            };

            OpenFileDialogInfo info = {};
            info.title              = "select shader source editor";
            info.filtersOrEmpty     = k_filters;
            FileDialog fd { info };
            if (fd.Open())
            {
                const StringView selectedPath = fd.GetFirstSelectedPath();
                m_sourceEditorPathOrEmpty     = selectedPath;
            }
        }

        ImGui::EndPopup();
    }

    // compile button
    ImGui::SameLine();
    const ImGuiKeyChord shortcut = GetShaderApp().GetShortCut(eShortCut::CompileShaderItem);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("compile", btnSize))
    {
        ShaderCompiler compiler = m_pFocusingShaderItemCache->Compile();
        if (compiler.IsFailed())
        {
            GetShaderApp().PushMessageDialog(
                "shader compile error",
                String::Formatted("failed to compile shader item \"{}\":\n\n{}", m_pFocusingShaderItemCache->name, compiler.GetErrorMessage()));
        }
        else
        {
            JAM_LOG_INFO("shader item \"{}\" compiled successfully", m_pFocusingShaderItemCache->name);
        }
    }
    Widget_ItemDescriptionTooltip("Compile Shader", "Compile the shader item and report any compilation errors.", shortcut);
    ImGui::EndDisabled();

#pragma endregion

    ImGui::End();
}

void ShaderItemEditorWindow::OnEvent(
    Event& event)
{
    EventDispatcher dispatcher { event };
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ShaderItemEditorWindow::OnProjectChangedEvent_);
    dispatcher.Dispatch<FocusingShaderItemChangedEvent>(this, &ShaderItemEditorWindow::OnFocusingShaderItemChangedEvent_);
    dispatcher.Dispatch<FileModifiedEvent>(this, &ShaderItemEditorWindow::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &ShaderItemEditorWindow::OnFileDeletedEvent_);
}

void ShaderItemEditorWindow::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.Write(m_sourceEditorPathOrEmpty);
}

Error ShaderItemEditorWindow::ReadFromArchive(
    ArchiveReader& _archive)
{
    return _archive.Read(m_sourceEditorPathOrEmpty);
}

void ShaderItemEditorWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent&)
{
    m_pProjectCache = GetShaderApp().GetProjectOrNull();
}

void ShaderItemEditorWindow::OnFocusingShaderItemChangedEvent_(
    const FocusingShaderItemChangedEvent&)
{
    m_pFocusingShaderItemCache = GetShaderApp().GetFocusingShaderItemOrNull();
    if (m_pFocusingShaderItemCache)
    {
        m_focusingShaderItemNameEdit       = m_pFocusingShaderItemCache->name;
        m_focusingShaderItemMacrosEdit     = m_pFocusingShaderItemCache->macros;
        m_focusingShaderItemEntryPointEdit = m_pFocusingShaderItemCache->entryPoint;
    }
}

void ShaderItemEditorWindow::OnFileModifiedEvent_(
    const FileModifiedEvent& _event) const
{
    if (!m_pProjectCache || !m_pFocusingShaderItemCache || !m_pFocusingShaderItemCache->pShaderSource)
    {
        return;
    }

    if (m_pFocusingShaderItemCache->pShaderSource->GetPath() == _event.GetPath())
    {
        if (const Error err = m_pFocusingShaderItemCache->pShaderSource->Load())
        {
            GetShaderApp().PushMessageDialog(
                "failed to reload shader source",
                String::Formatted("failed to reload shader source from file \"{}\". error: {}", _event.GetPath(), err.ToErrorMessage()));
        }
    }
}

void ShaderItemEditorWindow::OnFileDeletedEvent_(
    const FileDeletedEvent& _event) const
{
    if (!m_pProjectCache || !m_pFocusingShaderItemCache || !m_pFocusingShaderItemCache->pShaderSource)
    {
        return;
    }

    if (m_pFocusingShaderItemCache->pShaderSource->GetPath() == _event.GetPath())
    {
        m_pProjectCache->SetShaderItemSource(m_pFocusingShaderItemCache->name, nullptr);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemName()
{
    JAM_ASSERT(m_pProjectCache, "No project is currently loaded.");
    JAM_ASSERT(m_pFocusingShaderItemCache, "No shader item is currently being edited.");

    Widget_InputText("##ShaderItemNameEdit", m_focusingShaderItemNameEdit);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        // not allow duplicate typeName
        if (m_pFocusingShaderItemCache->name != m_focusingShaderItemNameEdit
            && m_pProjectCache->ContainsShaderItem(m_focusingShaderItemNameEdit))
        {
            GetShaderApp().PushMessageDialog(
                "invalid shader item typeName",
                String::Formatted("a shader item with typeName \"{}\" already exists. please choose a different typeName.", m_focusingShaderItemNameEdit));

            m_focusingShaderItemNameEdit = m_pFocusingShaderItemCache->name;
            return;
        }

        // not allow empty typeName
        if (m_focusingShaderItemNameEdit.IsEmpty())
        {
            GetShaderApp().PushMessageDialog(
                "invalid shader item typeName",
                "shader item typeName cannot be empty. please enter a valid typeName.");

            m_focusingShaderItemNameEdit = m_pFocusingShaderItemCache->name;
            return;
        }

        // rename shader item
        m_pProjectCache->RenameShaderItem(m_pFocusingShaderItemCache->name, m_focusingShaderItemNameEdit);
        GetShaderApp().SetFocusingShaderItem(m_focusingShaderItemNameEdit);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemShaderSource() const
{
    JAM_ASSERT(m_pFocusingShaderItemCache, "No shader item is currently being edited.");

    const Ref<ShaderSource>& pShaderSource = m_pFocusingShaderItemCache->pShaderSource;
    const ImVec2             frameSize     = { ImGui::CalcItemWidth(), ImGui::GetFrameHeight() };

    if (pShaderSource)
    {
        Widget_TextFrame(pShaderSource->GetPath().GetPtr(), frameSize, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);

        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(pShaderSource->GetPath().GetPtr());
            ImGui::EndTooltip();
        }
    }
    else
    {
        Widget_TextFrame("no shader source", frameSize, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);
    }

    ImGui::OpenPopupOnItemClick("ShaderItemShaderSourceEditContextMenu");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptPathDragDropPayload())
        {
            const String path = ResolvePathDragDropPayload(pPayload);
            if (const Expect<Ref<ShaderSource>> pLoadedShaderSource = GUI::AssetManager::LoadShaderSource(path))
            {
                m_pProjectCache->SetShaderItemSource(m_pFocusingShaderItemCache->name, pLoadedShaderSource.GetValue());
            }
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopup("ShaderItemShaderSourceEditContextMenu"))
    {
        if (ImGui::MenuItem("clear"))
        {
            m_pProjectCache->SetShaderItemSource(m_pFocusingShaderItemCache->name, nullptr);
        }

        ImGui::EndPopup();
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemEntryPoint()
{
    JAM_ASSERT(m_pFocusingShaderItemCache, "No shader item is currently being edited.");

    Widget_InputText("##ShaderItemEntryPointEdit", m_focusingShaderItemEntryPointEdit);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_pProjectCache->SetShaderItemEntryPoint(m_pFocusingShaderItemCache->name, m_focusingShaderItemEntryPointEdit);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemStage() const
{
    eShader stage = m_pFocusingShaderItemCache->stage;
    if (Widget_EnumCombo("##ShaderItemStageEdit", stage))
    {
        m_pProjectCache->SetShaderItemStage(m_pFocusingShaderItemCache->name, stage);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemCompileFlags() const
{
    JAM_ASSERT(m_pFocusingShaderItemCache, "No shader item is currently being edited.");

    constexpr EnumWidgetItem k_flagsItems[] = {
        {               "Debug info",             UnderlyingFrom(eShaderCompileFlags::DebugInfo) },
        {             "Optimization",          UnderlyingFrom(eShaderCompileFlags::Optimization) },
        {        "Skip optimization",      UnderlyingFrom(eShaderCompileFlags::SkipOptimization) },
        {       "Warning as errors ",      UnderlyingFrom(eShaderCompileFlags::WarningsAsErrors) },
        {       "Avoid flow control",      UnderlyingFrom(eShaderCompileFlags::AvoidFlowControl) },
        {    "Pack matrix row major",    UnderlyingFrom(eShaderCompileFlags::PackMatrixRowMajor) },
        { "Pack matrix column major", UnderlyingFrom(eShaderCompileFlags::PackMatrixColumnMajor) },
    };

    constexpr EnumWidgetItem k_setItems[] = {
        {            "None",           UnderlyingFrom(eShaderCompileFlags::None) },
        {   "Debug default",   UnderlyingFrom(eShaderCompileFlags::DebugDefault) },
        { "Release default", UnderlyingFrom(eShaderCompileFlags::ReleaseDefault) },
    };

    Int32 flags = NarrowCast<Int32>(m_pFocusingShaderItemCache->compileFlags.GetValue());
    if (Widget_EnumFlags("##ShaderItemCompileFlags", flags, k_flagsItems, k_setItems))
    {
        m_pProjectCache->SetShaderItemCompileFlags(m_pFocusingShaderItemCache->name, flags);
    }
}

void ShaderItemEditorWindow::OpenShaderSourceAsExternalEditor_(
    const StringView _path) const
{
    const bool bUseDefaultEditor = m_sourceEditorPathOrEmpty.IsEmpty();

    WString targetPath = {};
    targetPath.Append(L"\"");
    targetPath.AppendEncode(_path);
    targetPath.Append(L"\"");

    if (bUseDefaultEditor)
    {
        ::ShellExecuteW(NULL, L"open", targetPath.GetPtr(), NULL, NULL, SW_SHOWNORMAL);
    }
    else
    {
        WString editorPath = {};
        editorPath.Append(L"\"");
        editorPath.AppendEncode(m_sourceEditorPathOrEmpty.ToStringView());
        editorPath.Append(L"\"");
        ::ShellExecuteW(NULL, L"open", editorPath.GetPtr(), targetPath.GetPtr(), NULL, SW_SHOWNORMAL);
    }
}
