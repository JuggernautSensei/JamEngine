#include "pch.h"
#include "ShaderApp.h"

#include "EmbededFont.h"
#include "FileBrowserWindow.h"
#include "NewProjectDialog.h"
#include "ResourceManager.h"
#include "WorkspaceWindow.h"
#include "Events.h"
#include "../JamEngine/SVGImporter.h"
#include "../JamEngine/CommandLineArgumentParser.h"
#include "../JamEngine/MessageDialog.h"
#include "../JamEngine/ConfirmationDialog.h"
#include "../JamEngine/JsonArchive.h"

ShaderApp::ShaderApp(
    const StringView                _appName,
    const Flags<eAppSubSystemFlags> _subSystems)
    : Application(_appName, _subSystems)
{
}

void ShaderApp::Init(const Int32 _argc, char** _argv)
{
    CommandLineArgumentParser parser { _argc, _argv };
    for (Int32 i = 0; i < parser.GetNumArguments(); ++i)
    {
        const StringView arg = parser.GetArgument(i);
        JAM_LOG_INFO("Command line argument {}: '{}'", i, arg);
    }

    SetVsync(true);
    GetWindowsPlatform().AdjustClientRect(100, 100, 1600, 900);
    ResourceManager::CreateInstance();

    // set up
    SetupTheme_();
    SetupIcons_();
    SetupFont_();
    m_windowsOpen[eWindows::FileBrowser] = true;
    m_windowsOpen[eWindows::Workspace]   = true;

    // create windows
    m_pFileBrowserWindow = MakeScoped<FileBrowserWindow>();
    m_pWorkspaceWindow   = MakeScoped<WorkspaceWindow>();

    // load config if exists
    LoadConfigIfExists_();

    // load project if specified in command line arguments
    if (parser.GetNumArguments() == 3)
    {
        const StringView cmd = parser.GetArgument(1);

        if (cmd == "-proj")
        {
            const StringView path = parser.GetArgument(2);
            LoadProject(path);
        }
    }
}

void ShaderApp::Shutdown()
{
    ResourceManager::DestroyInstance();
}

void ShaderApp::Update(
    const float _dt)
{
    GetResourceManager().OnUpdate(_dt);
    UpdateAutoSaveConfig_(_dt);

    if (m_pProject)
    {
        m_pFileBrowserWindow->OnUpdate(_dt);
    }
}

void ShaderApp::ImGuiRender()
{
    GetGraphics().ClearColor(k_nullHandle, RGBA::k_black);

    Panel_Dockspace_();
    Panel_MainMenuBar_();

    if (m_pProject)
    {
        if (m_windowsOpen[eWindows::FileBrowser])
        {
            m_pFileBrowserWindow->OnImGuiRender();
        }

        if (m_windowsOpen[eWindows::Workspace])
        {
            m_pWorkspaceWindow->OnImGuiRender();
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
    dispatcher.Dispatch<WindowCloseEvent>(this, &ShaderApp::OnWindowClosedEvent_);
    dispatcher.Dispatch<ProjectChangedEvent>(this, &ShaderApp::OnProjectChagnedEvent_);
    dispatcher.Dispatch<ProjectRenamedEvent>(this, &ShaderApp::OnProjectRenamedEvent_);

    m_pFileBrowserWindow->OnEvent(_event);
}

void ShaderApp::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.WriteField("windowsOpen", m_windowsOpen);
}

Error ShaderApp::ReadFromArchive(
    ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("windowsOpen", m_windowsOpen))
    {
        return err;
    }

    return Error::None;
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
    for (eIcon e: EnumRanges<eIcon>())
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
    Int32    fontDataSize = NumericCast<Int32>(g_embeddedFontData.GetByteWidth());
    ImFont*  pFont        = io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 18, &fontConfig, io.Fonts->GetGlyphRangesKorean());
    JAM_ASSERT(pFont, "failed to load embedded font");
    io.FontDefault = pFont;
}

Ref<Texture> ShaderApp::LoadIconFromSVGSource_(
    const StringView _source) const
{
    SVGImporterInfo info = {};
    info.width           = 16;
    info.height          = 16;

    Expect<SVGImporter> importer = SVGImporter::ImportFromSource(_source, info);
    JAM_ASSERT(importer, "Failed to load SVG icon from source: {}", importer.GetError().ToErrorMessage());

    const CMemoryBlock  pixels[] = { importer->GetPixels() };
    const TextureHandle texh     = GetGraphics().CreateTexture2D(importer->GetWidth(), importer->GetHeight(), eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::None, pixels);
    return Texture::Create(texh);
}

void ShaderApp::OnProjectChagnedEvent_(
    const ProjectChangedEvent&) const
{
    UpdateAppTitle_();
}

void ShaderApp::OnProjectRenamedEvent_(
    const ProjectRenamedEvent&) const
{
    UpdateAppTitle_();
}

void ShaderApp::OnWindowClosedEvent_(const WindowCloseEvent& _event)
{
    Quit();   // todo.. project dirty check
}

void ShaderApp::UpdateAppTitle_() const
{
    if (m_pProject)
    {
        GetWindowsPlatform().SetTitle(String::Formatted("{} - {}", k_appName, m_pProject->GetName()));
    }
    else
    {
        GetWindowsPlatform().SetTitle(k_appName);
    }
}

void ShaderApp::NewProject_()
{
    NewProjectDialogInfo info = {};
    info.title                = "new project";
    PushDialog(MakeRef<NewProjectDialog>(info));
}

void ShaderApp::OpenProject_()
{
    constexpr FileDialogFilter filters[] = {
        { "JamShader Project File", Project::k_filter },
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

void ShaderApp::Panel_Dockspace_()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos);
    ImGui::SetNextWindowSize(pViewport->WorkSize);
    ImGui::SetNextWindowViewport(pViewport->ID);

    constexpr ImGuiWindowFlags k_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                                       | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

    bool bOpen = true;
    if (!ImGui::Begin("DockspaceWindow", &bOpen, k_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::PopStyleVar(3);

    m_dockspaceID                                 = ImGui::GetID("Dockspace");
    constexpr ImGuiDockNodeFlags k_dockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(m_dockspaceID, { 0.f, 0.f }, k_dockspaceFlags);
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
        if (ImGui::MenuItem("new project"))
        {
            NewProject_();
        }

        if (ImGui::MenuItem("open project"))
        {
            OpenProject_();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("exit"))
        {
            GetApp().Quit();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("windows"))
    {
        if (ImGui::MenuItem("file browser window", nullptr, m_windowsOpen[eWindows::FileBrowser]))
        {
            m_windowsOpen[eWindows::FileBrowser] = !m_windowsOpen[eWindows::FileBrowser];
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

    constexpr ImGuiWindowFlags k_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration;
    if (!ImGui::Begin("open project", nullptr, k_flags))
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
    if (const Error err = writer.SaveToFile(k_configFileName, eJsonWriteFlags::Pretty))
    {
        JAM_LOG_ERROR("Failed to save config file. {}", err.ToErrorMessage());
    }

    JAM_LOG_DEBUG("Config file saved.");
}

void ShaderApp::LoadConfig_()
{
    Expect<JsonReader> reader = JsonReader::ParseFromFile(k_configFileName);
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
    if (FileSystem::Exists(k_configFileName))
    {
        LoadConfig_();
    }
}

void ShaderApp::UpdateAutoSaveConfig_(
    const float _dt)
{
    m_configFileAutoSaveTimer += _dt;
    if (m_configFileAutoSaveTimer >= k_configFileAutoSaveInterval)
    {
        SaveConfig_();
        m_configFileAutoSaveTimer = 0.0f;
    }
}

void ShaderApp::SetIcon(
    const eIcon         _icon,
    const Ref<Texture>& _pTexture)
{
    icons[_icon] = _pTexture;
}

const Ref<Texture>& ShaderApp::GetIcon(
    const eIcon _icon) const
{
    return icons[_icon];
}

Error ShaderApp::LoadProject(
    const StringView _path)
{
    Expect<Ref<Project>> pProject = Project::LoadFromFile(_path);
    if (!pProject)
    {
        return pProject.GetError();
    }

    SetProject(pProject.GetValue());
    return Error::None;
}

void ShaderApp::SetProject(const Ref<Project>& _pProject)
{
    if (m_pProject == _pProject)
    {
        return;
    }

    m_pProject = _pProject;

    ProjectChangedEvent event { m_pProject };
    GetApp().DispatchEvent(event);
}

const Ref<Project>& ShaderApp::GetProject() const
{
    JAM_ASSERT(m_pProject, "Project is not loaded.");
    return m_pProject;
}

const Ref<Project>& ShaderApp::GetProjectOrNull() const
{
    return m_pProject;
}

void ShaderApp::PushDialog(
    const Ref<Dialog>& _pDialog)
{
    m_dialogStack.Push(_pDialog);
}

void ShaderApp::PushMessageDialog(
    const StringView _title,
    const StringView _message)
{
    MessageDialogInfo info = {};
    info.title             = _title;
    info.msg               = _message;
    m_dialogStack.Push(MakeRef<MessageDialog>(info));
}

void ShaderApp::PushConfirmationDialog(
    const StringView        _title,
    const StringView        _message,
    const Delegate<void()>& _onConfirm)
{
    ConfirmationDialogInfo info = {};
    info.title                  = _title;
    info.msg                    = _message;
    info.OnConfirm              = _onConfirm;
    m_dialogStack.Push(MakeRef<ConfirmationDialog>(info));
}
