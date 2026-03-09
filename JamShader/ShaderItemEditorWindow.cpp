#include "pch.h"
#include "ShaderItemEditorWindow.h"
#include "Events.h"
#include "GUIAssetManager.h"
#include "ShaderApp.h"
#include "../JamEngine/DirIter.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/FileDialog.h"
#include "../JamEngine/StackString.h"
#include "../JamEngine/STLAlgorithm.h"

void ShaderItemEditorWindow::OnImGuiRender()
{
    if (!ImGui::Begin("shader item editor"))
    {
        ImGui::End();
        return;
    }

    if (!m_pFocusingShaderItem)
    {
        Widget_TextFrame("no shader item selected", ImGui::GetContentRegionAvail());
        ImGui::End();
        return;
    }

    // shader item panel
    ImGui::SeparatorText("specification");
    if (ImGui::BeginTable("Specification", 2))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("name", true);
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

    ImGui::SeparatorText("pre-defined m_focusingShaderMacros");
    Widget_EditShaderItemMacros();

    ImGui::SeparatorText("shader file editor");
    Widget_ShaderSourceTextViewer(ImGui::GetContentRegionAvail());
    ImGui::End();
}

void ShaderItemEditorWindow::OnEvent(Event& event)
{
    EventDispatcher dispatcher { event };
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ShaderItemEditorWindow::OnProjectChangedEvent_);
    dispatcher.Dispatch<FocusingShaderItemChagnedEvent>(this, &ShaderItemEditorWindow::OnFocusingShaderItemChangedEvent_);
    dispatcher.Dispatch<FileModifiedEvent>(this, &ShaderItemEditorWindow::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &ShaderItemEditorWindow::OnFileDeletedEvent_);
}

void ShaderItemEditorWindow::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.WriteField("focusingShaderItemName", m_focusingShaderItemName);
}

Error ShaderItemEditorWindow::ReadFromArchive(
    ArchiveReader& _archive)
{
    return _archive.ReadField("focusingShaderItemName", m_focusingShaderItemName);
}

void ShaderItemEditorWindow::InitFocusingContext_(
    Project*         _pFocusingProjectOrNull,
    const StringView _focusingShaderItemOrEnpty)
{
    m_pProject = _pFocusingProjectOrNull;
    if (m_pProject)
    {
        m_focusingShaderItemName = _focusingShaderItemOrEnpty;
        m_pFocusingShaderItem    = m_focusingShaderItemName.IsEmpty() ? nullptr : &m_pProject->GetShaderItem(m_focusingShaderItemName);
    }
    else
    {
        ClearFocusingContext_();
    }
}

void ShaderItemEditorWindow::ClearFocusingContext_()
{
    m_pProject = nullptr;
    m_focusingShaderItemName.Clear();
    m_pFocusingShaderItem = nullptr;
}

void ShaderItemEditorWindow::OnProjectChangedEvent_(
    const ProjectChangedEvent& _event)
{
    InitFocusingContext_(_event.GetProjectOrNull(), {});
    ClearEditContext_();
}

void ShaderItemEditorWindow::OnFocusingShaderItemChangedEvent_(
    const FocusingShaderItemChagnedEvent& _event)
{
    if (!m_pProject)
    {
        ClearFocusingContext_();
        ClearEditContext_();
        return;
    }

    InitFocusingContext_(m_pProject, _event.GetFocusingShaderItemNameOrEmpty());
}

void ShaderItemEditorWindow::OnFileModifiedEvent_(
    const FileModifiedEvent& _event) const
{
    if (!m_pProject || !m_pFocusingShaderItem || !m_pFocusingShaderItem->pShaderSource)
    {
        return;
    }

    if (m_pFocusingShaderItem->pShaderSource->GetPath() == _event.GetPath())
    {
        if (const Error err = m_pFocusingShaderItem->pShaderSource->Load())
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
    if (!m_pProject || !m_pFocusingShaderItem || !m_pFocusingShaderItem->pShaderSource)
    {
        return;
    }

    if (m_pFocusingShaderItem->pShaderSource->GetPath() == _event.GetPath())
    {
        GetShaderApp().PushMessageDialog(
            "shader source file deleted",
            String::Formatted("the shader source file \"{}\" associated with the currently focused shader item has been deleted. the shader source reference has been cleared.", _event.GetPath()));
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemName()
{
    JAM_ASSERT(m_pProject, "No project is currently loaded.");

    Widget_InputText("##ShaderItemNameEdit", m_focusingShaderItemRenameEdit);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_pProject->RenameShaderItem(m_focusingShaderItemName, m_focusingShaderItemRenameEdit);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemShaderSource() const
{
    JAM_ASSERT(m_pFocusingShaderItem, "No shader item is currently being edited.");

    const Ref<ShaderSource>& pShaderSource = m_pFocusingShaderItem->pShaderSource;
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
                m_pProject->SetShaderItemSource(m_focusingShaderItemName, pLoadedShaderSource.GetValue());
            }
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopup("ShaderItemShaderSourceEditContextMenu"))
    {
        if (ImGui::MenuItem("clear"))
        {
            m_pProject->SetShaderItemSource(m_focusingShaderItemName, nullptr);
        }

        ImGui::EndPopup();
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemEntryPoint()
{
    JAM_ASSERT(m_pFocusingShaderItem, "No shader item is currently being edited.");

    Widget_InputText("##ShaderItemEntryPointEdit", m_focusingShaderItemEntryPointEdit);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_pProject->SetShaderItemEntryPoint(m_focusingShaderItemName, m_focusingShaderItemEntryPointEdit);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemStage() const
{
    eShader stage = m_pFocusingShaderItem->stage;
    if (Widget_EnumCombo("##ShaderItemStageEdit", stage))
    {
        m_pProject->SetShaderItemStage(m_focusingShaderItemName, stage);
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemCompileFlags() const
{
    JAM_ASSERT(m_pFocusingShaderItem, "No shader item is currently being edited.");

    struct CompileFlagInfo
    {
        eShaderCompileFlags flag;
        StringView          name;
    };

    constexpr Array<CompileFlagInfo, 7> k_compileFlagInfos = {
        {
         { eShaderCompileFlags::Debug, "Debug" },
         { eShaderCompileFlags::Optimization, "Optimization" },
         { eShaderCompileFlags::SkipOptimization, "Skip Optimization" },
         { eShaderCompileFlags::WarningsAsErrors, "Warnings As Errors" },
         { eShaderCompileFlags::AvoidFlowControl, "Avoid Flow Control" },
         { eShaderCompileFlags::PackMatrixRowMajor, "Pack Matrix Row Major" },
         { eShaderCompileFlags::PackMatrixColumnMajor, "Pack Matrix Column Major" },
         }
    };

    const ImVec2               btnSize = { ImGui::CalcItemWidth(), ImGui::GetFrameHeight() };
    Flags<eShaderCompileFlags> flags   = m_pFocusingShaderItem->compileFlags;

    // display compile flags as comma separated list
    StackString<256> buf;
    for (const CompileFlagInfo& info: k_compileFlagInfos)
    {
        if (flags.IsEnabled(info.flag))
        {
            if (!buf.IsEmpty())
            {
                buf.Append(", ");
            }
            buf.Append(info.name);
        }
    }
    ImGui::Button(buf.GetPtr(), btnSize);

    // Popup of compile flags with check box to toggle each flag
    const bool bTooltip = ImGui::IsItemHovered();
    if (ImGui::BeginPopupContextItem("ShaderItemCompileFlagsEditContextMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (const CompileFlagInfo& info: k_compileFlagInfos)
        {
            const bool bEnabled = flags.IsEnabled(info.flag);
            if (ImGui::MenuItem(info.name.GetPtr(), nullptr, bEnabled))
            {
                flags.Set(info.flag, !bEnabled);
                m_pProject->SetShaderItemCompileFlags(m_focusingShaderItemName, flags);
            }
        }

        if (ImGui::MenuItem("Debug default", nullptr))
        {
            m_pProject->SetShaderItemCompileFlags(m_focusingShaderItemName, eShaderCompileFlags::Debug);
        }

        if (ImGui::MenuItem("Release default", nullptr))
        {
            m_pProject->SetShaderItemCompileFlags(m_focusingShaderItemName, eShaderCompileFlags::Optimization);
        }

        ImGui::EndPopup();
    }

    // Tooltip for summarizing compile flags when hovering over the button
    if (bTooltip && ImGui::BeginItemTooltip())
    {
        for (const CompileFlagInfo& info: k_compileFlagInfos)
        {
            if (flags.IsEnabled(info.flag))
            {
                ImGui::BulletText(info.name.GetPtr());
            }
        }

        ImGui::EndTooltip();
    }
}

void ShaderItemEditorWindow::Widget_EditShaderItemMacros()
{
    const ImGuiStyle& style = ImGui::GetStyle();

    if (!ImGui::BeginChild("##ShaderItemMacrosChild", { 0.f, ImGui::GetFrameHeight() * 5.f }, ImGuiChildFlags_ResizeY))
    {
        ImGui::EndChild();
        return;
    }

    if (ImGui::BeginTable("##EditSHaderItemMacrosTable", 5, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("##Reordering", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("##Enabled", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
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
            Widget_Image(GetShaderApp().GetIcon(eIcon::ReorderShaderItemMaco), { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });

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
                                m_pProject->MoveShaderItemMacro(m_focusingShaderItemName, srcIdx, idx);
                            }));
                    }
                }

                ImGui::EndDragDropTarget();
            }

            // check box for macro enabled
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##MacroEnabledEdit", &bEnabled))
            {
                m_pProject->SetShaderItemMacro(m_focusingShaderItemName, idx, *it);
            }

            // Macro name
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            Widget_InputText("##MacroNameEdit", name);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                m_pProject->SetShaderItemMacro(m_focusingShaderItemName, idx, *it);
            }

            // Macros Value
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            Widget_InputText("##MacroValueEdit", value);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                m_pProject->SetShaderItemMacro(m_focusingShaderItemName, idx, *it);
            }

            // Macro clone button
            ImGui::TableNextColumn();
            if (Widget_ImageButton("Clone", GetShaderApp().GetIcon(eIcon::CloneShaderItemMacro), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
            {
                m_focusingShaderItemMacrosEdit.push_back(*it);
                m_pProject->PushShaderItemMacro(m_focusingShaderItemName, *it);
            }

            // Macro remove button
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            if (Widget_ImageButton("Remove", GetShaderApp().GetIcon(eIcon::RemoveShaderItemMacro), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
            {
                it = m_focusingShaderItemMacrosEdit.erase(it);
                m_pProject->RemoveShaderItemMacro(m_focusingShaderItemName, idx);
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    if (ImGui::Button("add new macro", CalcFitButtonSize()))
    {
        m_focusingShaderItemMacrosEdit.push_back({});
        m_pProject->PushShaderItemMacro(m_focusingShaderItemName, {});
    }
}

void ShaderItemEditorWindow::Widget_ShaderSourceTextViewer(
    const ImVec2 _size)
{
    JAM_ASSERT(m_pFocusingShaderItem, "No shader item is currently being edited.");

    // text editor
    ImVec2 childSize = _size;
    childSize.y -= ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y;
    if (ImGui::BeginChild("TextViewer", childSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoNavInputs))
    {
        if (m_pFocusingShaderItem->pShaderSource)
        {
            const StringView source = m_pFocusingShaderItem->pShaderSource->GetSource();
            Widget_ReadOnlyInputTextMultiline("##ShaderSourceTextEdit", source, ImGui::GetContentRegionAvail());
        }
        else
        {
            Widget_TextFrame("no shader source", ImGui::GetContentRegionAvail());
        }
    }
    ImGui::EndChild();

    // 2 buttons.
    const ImVec2             btnSize       = CalcFitButtonSize(2);
    const Ref<ShaderSource>& pShaderSource = m_pFocusingShaderItem->pShaderSource;

    // open as external editor
    ImGui::Separator();
    ImGui::BeginDisabled(!pShaderSource);
    if (ImGui::Button("open as", btnSize))
    {
        OpenShaderSourceAsExternalEditor_(pShaderSource->GetPath());
    }

    // choose open external editor
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("system default", 0, m_shaderSourceEditorPathOrEmpty.IsEmpty()))
        {
            m_shaderSourceEditorPathOrEmpty.Clear();
        }

        if (ImGui::MenuItem("choose manually", 0, !m_shaderSourceEditorPathOrEmpty.IsEmpty()))
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
                const StringView selectedPath   = fd.GetFirstSelectedPath();
                m_shaderSourceEditorPathOrEmpty = selectedPath;
            }
        }

        ImGui::EndPopup();
    }

    // compile
    ImGui::SameLine();
    ImGui::SetNextItemShortcut(GetShaderApp().GetShortCut(eShortCut::SaveProject));
    if (ImGui::Button("compile", btnSize))
    {
        ShaderCompiler compiler = m_pFocusingShaderItem->Compile();
        if (compiler.IsFailed())
        {
            GetShaderApp().PushMessageDialog(
                "shader compile error",
                String::Formatted("failed to compile shader item \"{}\":\n\n{}", m_focusingShaderItemName, compiler.GetErrorMessage()));
        }
        else
        {
            JAM_LOG_INFO("shader item \"{}\" compiled successfully", m_focusingShaderItemName);
        }
    }
    Widget_ItemDescriptionTooltip("Compile Shader", "Compile the shader item and report any compilation errors.");
    ImGui::EndDisabled();
}

void ShaderItemEditorWindow::OpenShaderSourceAsExternalEditor_(
    const StringView _path) const
{
    const bool bUseDefaultEditor = m_shaderSourceEditorPathOrEmpty.IsEmpty();

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
        editorPath.AppendEncode(m_shaderSourceEditorPathOrEmpty.ToStringView());
        editorPath.Append(L"\"");
        ::ShellExecuteW(NULL, L"open", editorPath.GetPtr(), targetPath.GetPtr(), NULL, SW_SHOWNORMAL);
    }
}

void ShaderItemEditorWindow::InitEditContext_(
    const StringView  _name,
    const ShaderItem* _pShaderItem)
{
    m_focusingShaderItemRenameEdit = _name;

    if (_pShaderItem)
    {
        m_focusingShaderItemMacrosEdit     = _pShaderItem->macros;
        m_focusingShaderItemEntryPointEdit = _pShaderItem->entryPoint;
    }
    else
    {
        m_focusingShaderItemMacrosEdit.clear();
        m_focusingShaderItemEntryPointEdit.Clear();
    }
}

void ShaderItemEditorWindow::ClearEditContext_()
{
    m_focusingShaderItemRenameEdit.Clear();
    m_focusingShaderItemMacrosEdit.clear();
    m_focusingShaderItemEntryPointEdit.Clear();
}