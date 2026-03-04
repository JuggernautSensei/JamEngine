#include "pch.h"

#include "Editor.h"

#include "Application.h"
#include "AssetManager.h"
#include "ConfirmationDialog.h"
#include "Editor_EntityInspectorWindow.h"
#include "Editor_FileBrowserWindow.h"
#include "Editor_ProfilerWindow.h"
#include "Editor_SceneHierarchyWindow.h"
#include "Editor_SceneViewWindow.h"
#include "Editor_VertexLayoutEditorWindow.h"
#include "Entity.h"
#include "DialogStack.h"
#include "EditorEvents.h"
#include "FileDialog.h"
#include "Graphics.h"
#include "MessageDialog.h"
#include "SVGImporter.h"
#include "JsonArchive.h"
#include "SceneManager.h"
#include "SceneNode.h"
#include "VertexLayoutEntryManager.h"
#include "Model.h"
#include "Profile.h"
#include "SceneSerializer.h"
#include "FileSystem.h"

namespace Jam
{

void Editor::OnSceneChanged_(
    SceneChangedEvent&)
{
    SetFocusingEntity(Entity::k_null);
}

void Editor::OnEntityDestroyed_(
    const EntityDestroyedEvent& _event)
{
    if (GetFocusingEntityOrNull() == _event.GetEntity())
    {
        SetFocusingEntity(Entity::k_null);
    }
}

void Editor::Menu_File_() const
{
    if (Widget_MenuItem("exit"))
    {
        GetApp().Quit();
    }
}

void Editor::Menu_Window_()
{
    Widget_MenuItem("scene view", 0, &GetWindowShow(eEditorWindowType::SceneView));
    Widget_MenuItem("scene hierarchy", 0, &GetWindowShow(eEditorWindowType::SceneHierarchy));
    Widget_MenuItem("entity inspector", 0, &GetWindowShow(eEditorWindowType::EntityInspector));
    Widget_MenuItem("file browser", 0, &GetWindowShow(eEditorWindowType::FileBrowser));
    Widget_MenuItem("profiler", 0, &GetWindowShow(eEditorWindowType::Profiler));
    Widget_MenuItem("vertex layout editor", 0, &GetWindowShow(eEditorWindowType::VertexLayoutEditor));
}

void Editor::Menu_Options_() const
{
    if (ImGui::BeginMenu("font scale"))
    {
        Widget_DragPercent("##FontScale", ImGui::GetIO().FontGlobalScale, 1.f, 50.f, 200.f);
        ImGui::EndMenu();
    }
}

void Editor::Panel_Dockspace_() const
{
    constexpr ImGuiWindowFlags k_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
                                       | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                       | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos);
    ImGui::SetNextWindowSize(pViewport->WorkSize);
    ImGui::SetNextWindowViewport(pViewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
    const bool bOpen = ImGui::Begin("workspace", nullptr, k_flags);
    ImGui::PopStyleVar(3);

    if (!bOpen)
    {
        ImGui::End();
        return;
    }

    ImGui::DockSpace(ImGui::GetID("Dockspace"));
    ImGui::End();
}

void Editor::Panel_MainMenuBar_()
{
    if (!ImGui::BeginMenuBar())
    {
        ImGui::EndMainMenuBar();
        return;
    }

    if (ImGui::BeginMenu("file"))
    {
        Menu_File_();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("window"))
    {
        Menu_Window_();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("options"))
    {
        Menu_Options_();
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void Editor::SetupDefaultTheme_() const
{
    // imgui m_style
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha                    = 1.0f;
        style.DisabledAlpha            = 0.6f;
        style.WindowPadding            = ImVec2(8.0f, 8.0f);
        style.WindowRounding           = 2.0f;
        style.WindowBorderSize         = 1.0f;
        style.WindowMinSize            = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding            = 2.0f;
        style.ChildBorderSize          = 1.0f;
        style.PopupRounding            = 2.0f;
        style.PopupBorderSize          = 1.0f;
        style.FramePadding             = ImVec2(6.0f, 2.0f);
        style.FrameRounding            = 2.0f;
        style.FrameBorderSize          = 1.0f;
        style.ItemSpacing              = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
        style.CellPadding              = ImVec2(4.0f, 2.0f);
        style.IndentSpacing            = 21.0f;
        style.ColumnsMinSpacing        = 6.0f;
        style.ScrollbarSize            = 14.0f;
        style.ScrollbarRounding        = 0.0f;
        style.GrabMinSize              = 14.0f;
        style.GrabRounding             = 2.0f;
        style.TabRounding              = 2.0f;
        style.TabBorderSize            = 0.0f;
        style.ColorButtonPosition      = ImGuiDir_Left;
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
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.8841113f, 0.8841166f, 0.88412017f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.44205567f, 0.44205886f, 0.4420601f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.3562196f, 0.35622218f, 0.35622317f, 1.0f);
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
        style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.0f, 0.35193133f, 0.35193133f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.944206f, 0.4538673f, 0.4538673f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(1.0f, 0.35193133f, 0.35193133f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.944206f, 0.4538673f, 0.4538673f, 1.0f);
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

    // imguizmo m_style
    {
        ImGuizmo::Style& style = ImGuizmo::GetStyle();

        style.TranslationLineThickness   = 3.0f * 1.5f;
        style.TranslationLineArrowSize   = 6.0f * 1.5f;
        style.RotationLineThickness      = 2.0f * 1.5f;
        style.RotationOuterLineThickness = 3.0f * 1.5f;
        style.ScaleLineThickness         = 3.0f * 1.5f;
        style.ScaleLineCircleSize        = 6.0f * 1.5f;
        style.HatchedAxisLineThickness   = 6.0f * 1.5f;
        style.CenterCircleSize           = 6.0f * 1.5f;

        style.Colors[ImGuizmo::DIRECTION_X]           = ImVec4(253 / 255.f, 54 / 255.f, 83 / 255.f, 1.f);
        style.Colors[ImGuizmo::DIRECTION_Y]           = ImVec4(117 / 255.f, 175 / 255.f, 24 / 255.f, 1.f);
        style.Colors[ImGuizmo::DIRECTION_Z]           = ImVec4(48 / 255.f, 128 / 255.f, 219 / 255.f, 1.f);
        style.Colors[ImGuizmo::PLANE_X]               = ImVec4(253 / 255.f, 54 / 255.f, 83 / 255.f, 97 / 255.f);
        style.Colors[ImGuizmo::PLANE_Y]               = ImVec4(117 / 255.f, 175 / 255.f, 24 / 255.f, 97 / 255.f);
        style.Colors[ImGuizmo::PLANE_Z]               = ImVec4(48 / 255.f, 128 / 255.f, 219 / 255.f, 97 / 255.f);
        style.Colors[ImGuizmo::SELECTION]             = ImVec4(1.000f, 0.500f, 0.062f, 0.541f);
        style.Colors[ImGuizmo::INACTIVE]              = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
        style.Colors[ImGuizmo::TRANSLATION_LINE]      = ImVec4(0.666f, 0.666f, 0.666f, 0.666f);
        style.Colors[ImGuizmo::SCALE_LINE]            = ImVec4(0.250f, 0.250f, 0.250f, 1.000f);
        style.Colors[ImGuizmo::ROTATION_USING_BORDER] = ImVec4(1.000f, 0.500f, 0.062f, 1.000f);
        style.Colors[ImGuizmo::ROTATION_USING_FILL]   = ImVec4(1.000f, 0.500f, 0.062f, 0.500f);
        style.Colors[ImGuizmo::HATCHED_AXIS_LINES]    = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
        style.Colors[ImGuizmo::TEXT]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
        style.Colors[ImGuizmo::TEXT_SHADOW]           = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
    }
}

void Editor::SetupIcons_()
{
    for (const eEditorIcon icon: EnumRanges<eEditorIcon>())
    {
        const StringView svgSource = g_embededEditorIconSVGSources[icon];
        SetIcon(icon, LoadIconFromSvgSource(svgSource));
    }
}

void Editor::UpdateAutoSaveConfig_(
    const float _deltaSec)
{
    m_configSaveCounterSec += _deltaSec;
    if (m_configSaveCounterSec >= Editor::k_configFileAutoSaveIntervalSec)
    {
        // save config only in edit m_mode
        if (GetMode() == eEditorMode::Edit)
        {
            SaveConfig_();
            m_configSaveCounterSec = 0.f;
        }
    }
}

void Editor::SaveConfig_() const
{
    JAM_FUNCTION_PROFILE();

    JsonWriter writer {};
    writer.WriteField("windowShows", m_windowShows);
    writer.WriteField("fontScale", ImGui::GetIO().FontGlobalScale);
    writer.WriteField("renderView", *m_pSceneViewWindow);
    writer.WriteField("fileBrowser", *m_pFileBrowserWindow);
    writer.WriteField("vertexLayoutMap", m_sharedData.vertexLayoutEntryManager);

    if (const Error err = writer.SaveToFile(k_configFileName, eJsonWriteFlags::Pretty))
    {
        JAM_LOG_WARN("Editor::SaveConfig_() - failed to save editor config file. {}.", err.ToErrorMessage());
    }

    JAM_LOG_DEBUG("Editor::SaveConfig_() - saved editor config file.");
}

void Editor::TryLoadConfig_()
{
    if (!FileSystem::Exists(k_configFileName))
    {
        return;
    }

    Expect<JsonReader> reader = JsonReader::ParseFromFile(k_configFileName);
    if (!reader)
    {
        return;
    }

    // general
    reader->ReadField("windowShows", m_windowShows);
    reader->ReadField("fontScale", ImGui::GetIO().FontGlobalScale);
    reader->ReadField("renderView", *m_pSceneViewWindow);
    reader->ReadField("fileBrowser", *m_pFileBrowserWindow);
    reader->ReadField("vertexLayoutMap", m_sharedData.vertexLayoutEntryManager);
}

Ref<Texture> Editor::LoadIconFromSvgSource(
    const StringView _svgSource) const
{
    SVGImporterInfo info = {};
    info.width           = k_iconSize.x;
    info.height          = k_iconSize.y;

    Expect<SVGImporter> importer = SVGImporter::ImportFromSource(_svgSource, info);
    if (!importer)
    {
        JAM_UNREACHABLE;
    }

    const CMemoryBlock  mems[] = { importer->GetPixels() };
    const Int32         w      = importer->GetWidth();
    const Int32         h      = importer->GetHeight();
    const TextureHandle texh   = GetGraphics().CreateTexture2D(w, h, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::None, mems);

    Ref<Texture> pTexture = {};
    pTexture->Load(texh);
    return pTexture;
}

bool& Editor::GetWindowShow(
    const eEditorWindowType _type)
{
    return m_windowShows[UnderlyingFrom(_type)];
}

}   // namespace Jam

namespace Jam
{

Editor& Editor::GetInstance()
{
    JAM_ASSERT(s_pInstance, "Editor::GetInstance() - editor instance is null.");
    return *s_pInstance;
}

void Editor::SetMode(
    const eEditorMode _mode)
{
    m_sharedData.mode = _mode;
}

eEditorMode Editor::GetMode() const
{
    return m_sharedData.mode;
}

void Editor::SetFocusingEntity(
    const Entity _entity)
{
    if (_entity == m_sharedData.focusingEntity)
    {
        return;
    }

    m_sharedData.focusingEntity = _entity;

    FocusingEntityChangedEvent event { _entity };
    GetApp().DispatchEvent(event);
}

Entity Editor::GetFocusingEntityOrNull() const
{
    return m_sharedData.focusingEntity;
}

Entity Editor::GetFocusingEntity() const
{
    JAM_ASSERT(m_sharedData.focusingEntity, "Editor::GetFocusingEntity() - focusing entity is null.");
    return m_sharedData.focusingEntity;
}

const EditorStyle& Editor::GetStyle() const
{
    return m_sharedData.style;
}

float Editor::GetStyleVar(
    const eEditorStyleVar _var) const
{
    return GetStyle().vars[_var];
}

ImVec4 Editor::GetStyleColor(
    const eEditorStyleColor _col) const
{
    return GetStyle().colors[_col];
}

ImU32 Editor::GetStyleColorU32(
    const eEditorStyleColor _col) const
{
    return ImGui::ColorConvertFloat4ToU32(GetStyleColor(_col));
}

ImGuiKeyChord Editor::GetStyleShortCut(
    const eEditorShortCut _shortCut) const
{
    return GetStyle().shortCuts[_shortCut];
}

Ref<Texture> Editor::GetIcon(
    const eEditorIcon _icon) const
{
    return m_sharedData.icons[_icon];
}

void Editor::SetIcon(
    const eEditorIcon   _icon,
    const Ref<Texture>& _pTexture)
{
    m_sharedData.icons[_icon] = _pTexture;
}

void Editor::SetFocusingFileEntry(const Ref<FileEntry>& _pEntry)
{
    if (_pEntry == m_sharedData.pFocusingFileEntry)
    {
        return;
    }

    m_sharedData.pFocusingFileEntry = _pEntry;
    FocusingFileEntryChangedEvent event { _pEntry };
    GetApp().DispatchEvent(event);
}

const Ref<FileEntry>& Editor::GetFocusingFileEntryOrNull() const
{
    return m_sharedData.pFocusingFileEntry;
}

const Ref<FileEntry>& Editor::GetFocusingFileEntry() const
{
    JAM_ASSERT(m_sharedData.pFocusingFileEntry, "Editor::GetFocusingFileEntry() - focusing file entry is null.");
    return m_sharedData.pFocusingFileEntry;
}

void Editor::SetFocusingDirEntry(const Ref<DirEntry>& _pEntry)
{
    if (_pEntry == m_sharedData.pFocusingDirEntry)
    {
        return;
    }

    m_sharedData.pFocusingDirEntry = _pEntry;
    FocusingDirEntryChangedEvent event { _pEntry };
    GetApp().DispatchEvent(event);
}

const Ref<DirEntry>& Editor::GetFocusingDirEntryOrNull() const
{
    return m_sharedData.pFocusingDirEntry;
}

const Ref<DirEntry>& Editor::GetFocusingDirEntry() const
{
    JAM_ASSERT(m_sharedData.pFocusingDirEntry, "Editor::GetFocusingDirEntry() - focusing dir entry is null.");
    return m_sharedData.pFocusingDirEntry;
}

VertexLayoutEntryManager& Editor::GetVertexLayoutManager()
{
    return m_sharedData.vertexLayoutEntryManager;
}

void Editor::PushDialog(
    const Ref<Dialog>& _pDialog)
{
    m_dialogStack.Push(_pDialog);
}

void Editor::PushMessageDialog(
    const StringView _title,
    const StringView _msg)
{
    MessageDialogInfo info = {};
    info.title             = _title;
    info.msg               = _msg;
    PushDialog(MakeRef<MessageDialog>(info));
}

void Editor::PushConfirmationDialog(
    const StringView        _title,
    const StringView        _msg,
    const Delegate<void()>& _onConfirm)
{
    ConfirmationDialogInfo info = {};
    info.title                  = _title;
    info.msg                    = _msg;
    info.OnConfirm              = _onConfirm;
    PushDialog(MakeRef<ConfirmationDialog>(info));
}

void Editor::LoadScenByFileDialog()
{
    constexpr FileDialogFilter k_filters[] = {
        { "scene file", Scene::k_filter }
    };

    OpenFileDialogInfo info = {};
    info.title              = "load scene";
    info.filtersOrEmpty     = k_filters;

    FileDialog fd { info };
    if (fd.Open())
    {
        StringView path = fd.GetFirstSelectedPath();
        if (!GetSceneManager().LoadScene(path))
        {
            PushMessageDialog(
                "error",
                String::Formatted("failed to load scene file: {}.", path));
        }
    }
}

void Editor::SaveSceneByFileDialog(
    const Ref<Scene>& _pScene)
{
    constexpr FileDialogFilter k_filters[] = {
        { "scene file", Scene::k_extension }
    };

    SaveFileDialogInfo info = {};
    info.title              = "save scene";
    info.filtersOrEmpty     = k_filters;

    FileDialog fd { info };
    if (fd.Open())
    {
        StringView path = fd.GetFirstSelectedPath();

        JsonWriter writer = {};
        SerializeScene(writer, _pScene);
        if (const Error err = writer.SaveToFile(path, eJsonWriteFlags::Pretty))
        {
            PushMessageDialog(
                "error",
                String::Formatted("failed to save scene file: {}.\n{}", path, err.ToErrorMessage()));
        }
    }
}

void Editor::CreateEntitiesFrom(
    const Ref<Scene>& _pScene,
    const Ref<Model>& _pModel)
{
    const Entity entity = _pScene->CreateEntitiesFrom(_pModel);
    SetFocusingEntity(entity);
}

void Editor::CreateEntity(
    const Ref<Scene>&      _pScene,
    const Optional<Entity> _parent)
{
    Entity entity = _pScene->CreateEntity();
    if (_parent)
    {
        SceneNode& node = entity.GetSceneNode();
        node.SetParent(_parent.GetValue());
    }
    SetFocusingEntity(entity);
}

void Editor::CloneEntity(
    const Ref<Scene>& _pScene,
    const Entity      _srcEntity)
{
    JAM_ASSERT(CanEntityClone(_srcEntity), "Editor::CloneEntity() - cannot clone the given entity.");
    const Entity entity = _pScene->CloneEntity(_srcEntity);
    SetFocusingEntity(entity);
}

void Editor::DestroyEntity(
    const Ref<Scene>& _pScene,
    const Entity      _entity)
{
    JAM_ASSERT(CanEntityDestroy(_entity), "Editor::DestroyEntity() - cannot destroy the given entity.");

    const SceneNode& node = _entity.GetSceneNode();
    if (node.HasChildren())   // if has children, show confirmation dialog
    {
        ConfirmationDialogInfo info = {};
        info.title                  = "confirmation";
        info.msg                    = "the entity has child entities. are you sure you want to destroy it and all its children?";
        info.OnConfirm              = Bind(
            [_pScene, _entity]
            {
                _pScene->DestroyEntity(_entity);
            });
        PushDialog(MakeRef<ConfirmationDialog>(info));
        return;
    }

    _pScene->DestroyEntity(_entity);
}

bool Editor::CanEntityClone(
    Entity _entity) const
{
    return _entity && !_entity.GetSceneNode().IsRoot();
}

bool Editor::CanEntityDestroy(
    Entity _entity) const
{
    return _entity && !_entity.GetSceneNode().IsRoot();
}

bool Editor::RemoveFile(
    const StringView _path,
    const bool       _bForced)
{
    // 만약, 디렉토리이고 비어있지 않다면 삭제 불가 -> 확인 다이얼로그 표시
    if (_path.IsDirectory() && !FileSystem::IsEmpty(_path) && !_bForced)
    {
        const Delegate<void()> OnConfirm = Bind(
            [_path]
            {
                GetEditor().RemoveFile(_path, true);
            });

        PushConfirmationDialog(
            "confirmation",
            String::Formatted("the directory is not empty. are you sure you want to delete it and all its contents?"),
            OnConfirm);

        return false;
    }

    // 빈 디렉토리 혹은 파일인 경우 바로 삭제
    if (const Error err = FileSystem::Remove(_path))
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to delete {}.\n{}", _path, err.ToErrorMessage()));

        return false;
    }

    return true;
}

bool Editor::RenameFile(
    const StringView                   _srcPath,
    const StringView                   _dstPath,
    const FileSystem::eCollisionPolicy _policy)
{
    // same path
    if (_srcPath == _dstPath)
    {
        return true;
    }

    // if dst path already exists, show confirmation dialog
    if (const Error err = FileSystem::Rename(_srcPath, _dstPath, _policy))
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to rename {} to {}.\n{}", _srcPath, _dstPath, err.ToErrorMessage()));

        return false;
    }

    return true;
}

bool Editor::CreateDir(
    const StringView                                _dir,
    const FileSystem::eMakeDirectoryCollisionPolicy _policy)
{
    JAM_ASSERT(_dir.IsDirectory(), "Editor::MakeDirectory() - the given path is not a directory.");

    if (const Error err = FileSystem::MakeDirectory(_dir, _policy))
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to create directory: {}.\n{}", _dir, err.ToErrorMessage()));

        return false;
    }
    return true;
}

Expect<Ref<Mesh>> Editor::LoadMesh(
    StringView _path,
    const bool _bForceReload)
{
    Expect<Ref<Mesh>> pMesh = GetAssetManager().LoadMesh(_path, _bForceReload);
    if (!pMesh)
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to load mesh file: {}.\n{}", _path, pMesh.GetError().ToErrorMessage()));
    }
    return pMesh;
}

Expect<Ref<Texture>> Editor::LoadTexture(
    StringView _path,
    const bool _bForceReload)
{
    Expect<Ref<Texture>> pTexture = GetAssetManager().LoadTexture(_path, _bForceReload);
    if (!pTexture)
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to load texture file: {}.\n{}", _path, pTexture.GetError().ToErrorMessage()));
    }
    return pTexture;
}

Expect<Ref<Material>> Editor::LoadMaterial(
    StringView _path,
    const bool _bForceReload)
{
    Expect<Ref<Material>> pMaterial = GetAssetManager().LoadMaterial(_path, _bForceReload);
    if (!pMaterial)
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to load material file: {}.\n{}", _path, pMaterial.GetError().ToErrorMessage()));
    }
    return pMaterial;
}

Expect<Ref<Model>> Editor::LoadModel(
    StringView _path,
    const bool _bForceReload)
{
    Expect<Ref<Model>> pModel = GetAssetManager().LoadModel(_path, _bForceReload);
    if (!pModel)
    {
        PushMessageDialog(
            "error",
            String::Formatted("failed to load model file: {}.\n{}", _path, pModel.GetError().ToErrorMessage()));
    }
    return pModel;
}

Editor::Editor()
    : m_pSceneViewWindow(MakeScoped<Editor_SceneViewWindow>())
    , m_pProfilerWindow(MakeScoped<Editor_ProfilerWindow>())
    , m_pSceneHierarchyWindow(MakeScoped<Editor_SceneHierarchyWindow>())
    , m_pEntityInspectorWindow(MakeScoped<Editor_EntityInspectorWindow>())
    , m_pFileBrowserWindow(MakeScoped<Editor_FileBrowserWindow>())
    , m_pVertexLayoutEditorWindow(MakeScoped<Editor_VertexLayoutEditorWindow>())
{
    SetupDefaultTheme_();
    SetupIcons_();
    TryLoadConfig_();
}

Editor::~Editor()
{
    SaveConfig_();
}

void Editor::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "Editor instance already exists!");
    s_pInstance = new Editor;
}

void Editor::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

void Editor::Update(const float _deltaSec)
{
    // auto save config
    UpdateAutoSaveConfig_(_deltaSec);

    // update render view
    m_pSceneViewWindow->OnUpdate(_deltaSec);
}

void Editor::Render() const
{
    m_pSceneViewWindow->OnRender();
}

void Editor::ImGuiRender()
{
    Panel_Dockspace_();

    // viewport
    if (bool& bShow = GetWindowShow(eEditorWindowType::SceneView))
    {
        m_pSceneViewWindow->OnImGuiRender(bShow);
    }

    ImGui::BeginDisabled(GetMode() == eEditorMode::Play);

    // profiler
    if (bool& bShow = GetWindowShow(eEditorWindowType::Profiler))
    {
        m_pProfilerWindow->OnImGuiRender(bShow);
    }

    // scene hierarchy
    if (bool& bShow = GetWindowShow(eEditorWindowType::SceneHierarchy))
    {
        m_pSceneHierarchyWindow->OnImGuiRender(bShow);
    }

    // entity inspector
    if (bool& bShow = GetWindowShow(eEditorWindowType::EntityInspector))
    {
        m_pEntityInspectorWindow->OnImGuiRender(bShow);
    }

    // file system browser
    if (bool& bShow = GetWindowShow(eEditorWindowType::FileBrowser))
    {
        m_pFileBrowserWindow->OnImGuiRender(bShow);
    }

    // vertex vertexLayout manager
    if (bool& bShow = GetWindowShow(eEditorWindowType::VertexLayoutEditor))
    {
        m_pVertexLayoutEditorWindow->OnImGuiRender(bShow);
    }

    ImGui::EndDisabled();

    // show dialog
    m_dialogStack.Show();
}

void Editor::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<SceneChangedEvent>(this, &Editor::OnSceneChanged_);
    dispatcher.Dispatch<EntityDestroyedEvent>(this, &Editor::OnEntityDestroyed_);
    m_pFileBrowserWindow->OnEvent(_event);
    m_pSceneViewWindow->OnEvent(_event);
}

}   // namespace Jam
