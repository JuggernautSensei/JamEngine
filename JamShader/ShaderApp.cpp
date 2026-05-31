#include "pch.h"
#include "ShaderApp.h"

#include "EmbededFont.h"
#include "FileBrowserWindow.h"
#include "NewProjectDialog.h"
#include "ResourceManager.h"
#include "Events.h"
#include "ExportProjectDialog.h"
#include "../JamEngine/SVGImporter.h"
#include "../JamEngine/CommandLineParser.h"
#include "../JamEngine/MessageDialog.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/JsonArchive.h"

ShaderApp::ShaderApp(
    const StringView           _appName,
    const Flags<eAppInitFlags> _subSystems)
    : Application(_appName, _subSystems)
{
}

void ShaderApp::SetIcon(
    const eIcon         _icon,
    const Ref<Texture>& _pTexture)
{
    m_icons[_icon] = _pTexture;
}

const Ref<Texture>& ShaderApp::GetIcon(
    const eIcon _icon) const
{
    return m_icons[_icon];
}

void ShaderApp::SetShortCut(
    const eShortCut     _shortCut,
    const ImGuiKeyChord _key)
{
    m_shortcuts[_shortCut] = _key;
}

ImGuiKeyChord ShaderApp::GetShortCut(
    const eShortCut _shortCut) const
{
    return m_shortcuts[_shortCut];
}

Error ShaderApp::LoadProject(
    const StringView _path)
{
    Expect<Project> pProject = Project::Load(_path);
    if (!pProject)
    {
        return pProject.GetError();
    }

    SetProject(pProject.GetValue());
    return Error::None;
}

void ShaderApp::SetProject(
    const Project& _pProject)
{
    m_project = _pProject;

    ProjectChangedEvent event {};
    app.DispatchEvent(event);
}

void ShaderApp::ReleaseProject()
{
    if (m_project)
    {
        m_project.Reset();

        ProjectChangedEvent event {};
        app.DispatchEvent(event);
    }
}

void ShaderApp::SetFocusingShaderItem(
    const StringView _name)
{
    JAM_ASSERT(m_project, "No project is loaded.");

    if (m_focusingShaderItemName == _name)
    {
        return;
    }

    m_focusingShaderItemName = _name;
    FocusingShaderItemChangedEvent event {};
    app.DispatchEvent(event);
}

void ShaderApp::ReleaseFocusingShaderItem()
{
    SetFocusingShaderItem({});
}

Project* ShaderApp::GetProjectOrNull()
{
    return m_project ? &m_project.GetValue() : nullptr;
}

const ShaderItem* ShaderApp::GetFocusingShaderItemOrNull() const
{
    if (!m_project)
    {
        return nullptr;
    }

    return m_focusingShaderItemName.IsEmpty() ? nullptr : &m_project.GetValue().GetShaderItem(m_focusingShaderItemName);
}

void ShaderApp::PushDialog(
    Scoped<Dialog>&& _pDialog)
{
    m_dialogStack.Push(std::move(_pDialog));
}

void ShaderApp::PushMessageDialog(
    const StringView _title,
    const StringView _message)
{
    MessageDialogButtonInfo buttons[] = {
        { "ok", nullptr }
    };

    MessageDialogInfo info = {};
    info.title             = _title;
    info.message           = _message;
    info.buttons           = buttons;
    m_dialogStack.Push(MakeScoped<MessageDialog>(info));
}

void ShaderApp::PushConfirmationDialog(
    const StringView        _title,
    const StringView        _message,
    const Delegate<void()>& _onConfirm)
{
    const MessageDialogButtonInfo buttons[] = {
        { "confirm", _onConfirm },
        {  "cancel",    nullptr }
    };

    MessageDialogInfo info = {};
    info.title             = _title;
    info.message           = _message;
    info.buttons           = buttons;
    m_dialogStack.Push(MakeScoped<MessageDialog>(info));
}

DialogStack& ShaderApp::GetDialogStack()
{
    return m_dialogStack;
}

void ShaderApp::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.BeginObject();
    _archive.WriteField("windowsOpen", m_windowsOpen);
    _archive.WriteField("shaderItemEditorWindow", m_shaderItemEditorWindow);
    _archive.EndObject();
}

Error ShaderApp::ReadFromArchive(
    ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("windowsOpen", m_windowsOpen))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("shaderItemEditorWindow", m_shaderItemEditorWindow))
    {
        return err;
    }

    return Error::None;
}

void ShaderApp::Init(const Int32 _argc, char** _argv)
{
    CommandLineParser parser { _argc, _argv };
    for (Int32 i = 0; i < parser.GetNumArguments(); ++i)
    {
        const StringView arg = parser.GetArgument(i);
        JAM_LOG_INFO("Command line argument {}: '{}'", i, arg);
    }

    ResourceManager::CreateInstance();

    SetVsync(true);
    GetWindowsPlatform().AdjustClientRect(100, 100, 1600, 900);

    // set up
    SetupTheme_();
    SetupIcons_();
    SetupFont_();
    SetupShortCuts_();
    m_windowsOpen[eWindows::FileBrowser]      = true;
    m_windowsOpen[eWindows::ProjectInspector] = true;
    m_windowsOpen[eWindows::ShaderItemEditor] = true;

    // load config if exists
    LoadConfigIfExists_();

    // load project if specified in command line arguments
    if (parser.GetNumArguments() == 2)
    {
        const StringView path = parser.GetArgument(1);
        LoadProject(path);
    }
}

void ShaderApp::Shutdown()
{
    SaveConfig_();

    m_dialogStack.Clear();   // release manually (because FileDialog and ComPtr)
    m_icons.Fill(nullptr);   // release hardware textures
    ResourceManager::DestroyInstance();
}

void ShaderApp::Render()
{
}

void ShaderApp::Update(
    const float _dt)
{
    GetResourceManager().OnUpdate(_dt);
    UpdateAutoSaveConfig_(_dt);

    if (m_project)
    {
        m_fileBrowserWindow.OnUpdate(_dt);
    }
}

void ShaderApp::ImGuiRender()
{
    gfx.ClearRenderTarget(kInvalidHandle, RGBA::kBlack);

    GlobalShortCut_();
    Panel_Dockspace_();
    Panel_MainMenuBar_();

    if (m_project)
    {
        if (m_windowsOpen[eWindows::FileBrowser])
        {
            m_fileBrowserWindow.OnImGuiRender();
        }

        if (m_windowsOpen[eWindows::ProjectInspector])
        {
            m_projectInspectorWindow.OnImGuiRender();
        }

        if (m_windowsOpen[eWindows::ShaderItemEditor])
        {
            m_shaderItemEditorWindow.OnImGuiRender();
        }
    }
    else
    {
        Panel_OpenProject_();
    }

    m_dialogStack.Show();
}

void ShaderApp::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ShaderApp::OnProjectChagnedEvent_);
    dispatcher.Dispatch<ProjectRenamedEvent>(this, &ShaderApp::OnProjectRenamedEvent_);
    dispatcher.Dispatch<ProjectDirtyChangedEvent>(this, &ShaderApp::OnProjectDirtyChangedEvent_);
    dispatcher.Dispatch<WindowClosedEvent>(this, &ShaderApp::OnWindowClosedEvent_);

    GetResourceManager().OnEvent(_event);
    m_fileBrowserWindow.OnEvent(_event);
    m_projectInspectorWindow.OnEvent(_event);
    m_shaderItemEditorWindow.OnEvent(_event);
}

void ShaderApp::SetupTheme_() const
{

    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 0.6f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 8.4f;
    style.WindowBorderSize         = 1.0f;
    style.WindowMinSize            = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding            = 3.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 3.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(4.0f, 3.0f);
    style.FrameRounding            = 3.0f;
    style.FrameBorderSize          = 1.0f;
    style.ItemSpacing              = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
    style.CellPadding              = ImVec2(4.0f, 2.0f);
    style.IndentSpacing            = 21.0f;
    style.ColumnsMinSpacing        = 6.0f;
    style.ScrollbarSize            = 5.6f;
    style.ScrollbarRounding        = 18.0f;
    style.GrabMinSize              = 10.0f;
    style.GrabRounding             = 3.0f;
    style.TabRounding              = 3.0f;
    style.TabBorderSize            = 0.0f;
    style.ColorButtonPosition      = ImGuiDir_Right;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.1254902f, 0.1254902f, 0.1254902f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.34901962f, 0.34901962f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.2509804f, 0.2509804f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.21568628f, 0.21568628f, 0.21568628f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.0f, 0.32941177f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.0f, 0.47058824f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

void ShaderApp::SetupIcons_()
{
    for (eIcon e: RangesOf<eIcon>())
    {
        SetIcon(e, LoadIconFromSVGSource_(g_embededIconSVGResources[e]));
    }
}

void ShaderApp::SetupFont_() const
{
    ImFontConfig fontConfig         = {};
    fontConfig.FontDataOwnedByAtlas = false;

    ImGuiIO& io           = ImGui::GetIO();
    void*    fontData     = const_cast<void*>(static_cast<const void*>(g_embeddedFontData.GetPtr()));
    Int32    fontDataSize = NarrowCast<Int32>(g_embeddedFontData.GetByteWidth());
    ImFont*  pFont        = io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 18, &fontConfig, io.Fonts->GetGlyphRangesKorean());
    JAM_ASSERT(pFont, "failed to load embedded font");
    io.FontDefault = pFont;
}

void ShaderApp::SetupShortCuts_()
{
    m_shortcuts[eShortCut::NewProject]        = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N;
    m_shortcuts[eShortCut::OpenProject]       = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O;
    m_shortcuts[eShortCut::SaveProject]       = ImGuiMod_Ctrl | ImGuiKey_S;
    m_shortcuts[eShortCut::SaveAsProject]     = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S;
    m_shortcuts[eShortCut::ExitApp]           = ImGuiMod_Alt | ImGuiKey_F4;
    m_shortcuts[eShortCut::CompileShaderItem] = ImGuiKey_F5;
    m_shortcuts[eShortCut::ExportProject]     = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E;
}

Ref<Texture> ShaderApp::LoadIconFromSVGSource_(
    const StringView _source) const
{
    SVGImporterInfo info = {};
    info.width           = 16;
    info.height          = 16;

    Expect<SVGImporter> importer = SVGImporter::ImportFromSource(_source, info);
    JAM_ASSERT(importer, "Failed to load SVG icon from source: {}", importer.GetError().ToErrorMessage());

    const MemoryView    pixels[] = { importer->GetPixels() };
    const TextureHandle texh     = gfx.CreateTexture2D(importer->GetWidth(), importer->GetHeight(), eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::None, pixels);
    return Texture::Load(texh);
}

void ShaderApp::OnProjectChagnedEvent_(
    const ProjectChangedEvent&)
{
    UpdateAppTitle_();
    ReleaseFocusingShaderItem();
}

void ShaderApp::OnProjectRenamedEvent_(
    const ProjectRenamedEvent&) const
{
    UpdateAppTitle_();
}

void ShaderApp::OnProjectDirtyChangedEvent_(
    const ProjectDirtyChangedEvent&) const
{
    UpdateAppTitle_();
}

void ShaderApp::OnWindowClosedEvent_(
    const WindowClosedEvent&)
{
    TryQuit_();
}

void ShaderApp::UpdateAppTitle_() const
{
    if (m_project)
    {
        GetWindowsPlatform().SetTitle(
            String::Formatted("{} - {}{}",
                              kAppName,
                              m_project->GetName(),
                              m_project->IsDirty() ? "*" : ""));
    }
    else
    {
        GetWindowsPlatform().SetTitle(kAppName);
    }
}

void ShaderApp::NewProject_()
{
    PushDialog(MakeScoped<NewProjectDialog>());
}

void ShaderApp::OpenProject_()
{
    constexpr FileDialogFilter filters[] = {
        { "JamShader Project File", Project::kFilter },
    };

    OpenFileDialogInfo info = {};
    info.title              = "open project";
    info.filtersOrEmpty     = filters;

    FileDialog dlg { info };
    if (dlg.Open())
    {
        const StringView path = dlg.GetFirstSelectedPath();
        LoadProject(path);
    }
}

void ShaderApp::SaveProject_()
{
    JAM_ASSERT(m_project, "No project to save.");

    if (const Error err = m_project->Save())
    {
        PushMessageDialog("failed to save project", String::Formatted("failed to save project. {}", err.ToErrorMessage()));
    }
}

void ShaderApp::SaveAsProject_()
{
    constexpr FileDialogFilter filters[] = {
        { "JamShader Project File", Project::kFilter },
    };

    SaveFileDialogInfo info = {};
    info.title              = "open project";
    info.filtersOrEmpty     = filters;

    FileDialog dlg { info };
    if (dlg.Open())
    {
        const StringView path = dlg.GetFirstSelectedPath();
        if (const Error err = m_project->SaveAs(path))
        {
            PushMessageDialog("failed to save project", String::Formatted("failed to save project. {}", err.ToErrorMessage()));
        }
    }
}

void ShaderApp::Panel_Dockspace_()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos);
    ImGui::SetNextWindowSize(pViewport->WorkSize);
    ImGui::SetNextWindowViewport(pViewport->ID);

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                                       | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

    bool bOpen = true;
    if (!ImGui::Begin("DockspaceWindow", &bOpen, kFlags))
    {
        ImGui::PopStyleVar(3);
        ImGui::End();
        return;
    }

    ImGui::PopStyleVar(3);

    m_dockspaceID                                 = ImGui::GetID("Dockspace");
    constexpr ImGuiDockNodeFlags kDockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(m_dockspaceID, { 0.f, 0.f }, kDockspaceFlags);
    ImGui::End();
}

void ShaderApp::Panel_MainMenuBar_()
{
    if (!ImGui::BeginMainMenuBar())
    {
        ImGui::EndMainMenuBar();
        return;
    }

    if (ImGui::BeginMenu("file"))
    {
        if (Widget_MenuItem("new project", GetShortCut(eShortCut::NewProject)))
        {
            NewProject_();
        }

        if (Widget_MenuItem("open project", GetShortCut(eShortCut::OpenProject)))
        {
            OpenProject_();
        }

        ImGui::Separator();
        if (Widget_MenuItem("save project", GetShortCut(eShortCut::SaveProject)))
        {
            SaveProject_();
        }

        if (Widget_MenuItem("save project as", GetShortCut(eShortCut::SaveAsProject)))
        {
            SaveAsProject_();
        }

        if (Widget_MenuItem("export project", GetShortCut(eShortCut::ExportProject)))
        {
            ExportProject_();
        }

        ImGui::Separator();
        if (Widget_MenuItem("exit", GetShortCut(eShortCut::ExitApp)))
        {
            TryQuit_();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("windows"))
    {
        if (ImGui::MenuItem("file browser window", nullptr, m_windowsOpen[eWindows::FileBrowser]))
        {
            m_windowsOpen[eWindows::FileBrowser] = !m_windowsOpen[eWindows::FileBrowser];
        }

        if (ImGui::MenuItem("project inspector", nullptr, m_windowsOpen[eWindows::ProjectInspector]))
        {
            m_windowsOpen[eWindows::ProjectInspector] = !m_windowsOpen[eWindows::ProjectInspector];
        }

        if (ImGui::MenuItem("shader item editor window", nullptr, m_windowsOpen[eWindows::ShaderItemEditor]))
        {
            m_windowsOpen[eWindows::ShaderItemEditor] = !m_windowsOpen[eWindows::ShaderItemEditor];
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void ShaderApp::Panel_OpenProject_()
{
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(pViewport->WorkSize, ImGuiCond_Always);

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration;
    if (!ImGui::Begin("open project", nullptr, kFlags))
    {
        ImGui::End();
        return;
    }

    // draw "new project", "open project" buttons at the center of the screen
    const ImGuiStyle& style     = ImGui::GetStyle();
    const ImVec2      btnSize   = { 200.f, ImGui::GetFrameHeight() };
    const float       spacingY  = style.ItemSpacing.y * 2;
    const ImVec2      totalSize = { btnSize.x, btnSize.y * 2 + spacingY };
    const ImVec2      cursor    = ImGui::GetWindowPos() + (ImGui::GetContentRegionAvail() - totalSize) * 0.5f;

    ImGui::SetCursorScreenPos(cursor);
    if (ImGui::Button("new project", btnSize))
    {
        NewProject_();
    }

    ImGui::SetCursorScreenPos({ cursor.x, cursor.y + btnSize.y + spacingY });
    if (ImGui::Button("open project", btnSize))
    {
        OpenProject_();
    }

    ImGui::End();
}

void ShaderApp::SaveConfig_() const
{
    JsonWriter writer = {};
    writer.Write(*this);
    if (const Error err = writer.SaveToFile(kConfigFileName, eJsonWriteFlags::Pretty))
    {
        JAM_LOG_ERROR("Failed to save config file. {}", err.ToErrorMessage());
    }

    JAM_LOG_DEBUG("Config file saved.");
}

void ShaderApp::LoadConfig_()
{
    Expect<JsonReader> reader = JsonReader::ParseFromFile(kConfigFileName);
    if (!reader)
    {
        JAM_LOG_ERROR("Failed to load config file. {}", reader.GetError().ToErrorMessage());
        return;
    }

    if (const Error err = reader->Read(*this))
    {
        JAM_LOG_ERROR("Failed to read config file. {}", err.ToErrorMessage());
    }
}

void ShaderApp::LoadConfigIfExists_()
{
    if (FileSystem::Exists(kConfigFileName))
    {
        LoadConfig_();
    }
}

void ShaderApp::UpdateAutoSaveConfig_(
    const float _dt)
{
    m_configFileAutoSaveTimer += _dt;
    if (m_configFileAutoSaveTimer >= kConfigFileAutoSaveInterval)
    {
        SaveConfig_();
        m_configFileAutoSaveTimer = 0.0f;
    }
}

void ShaderApp::GlobalShortCut_()
{
    if (ImGui::Shortcut(GetShortCut(eShortCut::OpenProject), ImGuiInputFlags_RouteGlobal))
    {
        OpenProject_();
    }

    if (ImGui::Shortcut(GetShortCut(eShortCut::NewProject), ImGuiInputFlags_RouteGlobal))
    {
        NewProject_();
    }

    if (ImGui::Shortcut(GetShortCut(eShortCut::ExitApp), ImGuiInputFlags_RouteGlobal))
    {
        TryQuit_();
    }

    if (m_project)
    {
        if (ImGui::Shortcut(GetShortCut(eShortCut::SaveProject), ImGuiInputFlags_RouteGlobal))
        {
            SaveProject_();
        }

        if (ImGui::Shortcut(GetShortCut(eShortCut::SaveAsProject), ImGuiInputFlags_RouteGlobal))
        {
            SaveAsProject_();
        }

        if (ImGui::Shortcut(GetShortCut(eShortCut::ExportProject), ImGuiInputFlags_RouteGlobal))
        {
            ExportProject_();
        }
    }
}

void ShaderApp::TryQuit_()
{
    if (!m_project || !m_project->IsDirty())
    {
        Quit();
        return;
    }

    const String                  message   = String::Formatted("There are unsaved changes in the current project '{}'. Are you sure you want to quit?", m_project->GetName());
    const MessageDialogButtonInfo buttons[] = {
        {       "save and quit", Bind<&ShaderApp::SaveAndQuit_>(this) },
        { "quit without saving",         Bind<&ShaderApp::Quit>(this) },
        {              "cancel",                              nullptr }
    };

    MessageDialogInfo info = {};
    info.title             = "unsaved changes";
    info.message           = message;
    info.buttons           = buttons;
    info.size              = { 700.f, 0.f };
    PushDialog(MakeScoped<MessageDialog>(info));
}

void ShaderApp::SaveAndQuit_()
{
    if (m_project)
    {
        SaveProject_();
    }

    Quit();
}

void ShaderApp::ExportProject_()
{
    JAM_ASSERT(m_project, "No project to export.");
    PushDialog(MakeScoped<ExportProjectDialog>(&m_project.GetValue()));
}
