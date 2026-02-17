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
#include "Graphics.h"
#include "MessageDialog.h"
#include "SVGImporter.h"
#include "JamEngineEditorEmbededIcons.h"
#include "JsonArchive.h"
#include "SceneManager.h"
#include "SceneNode.h"
#include "VertexLayoutEntry.h"
#include "Model.h"
#include "Profile.h"
#include "SceneSerializer.h"

namespace Jam
{

void Editor::OnSceneChanged_(
    SceneChangedEvent&)
{
    m_focusedEntity = Entity::k_invalid;
}

void Editor::OnEntityDestroyed_(
    const EntityDestroyedEvent& _event)
{
    if (m_focusedEntity.GetUID() == _event.GetEntityUID())
    {
        m_focusedEntity = Entity::k_invalid;
    }
}

void Editor::Menu_File_() const
{
    if (Widget_MenuItem("exit"))
    {
        JAM_APP.Quit();
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

void Editor::ShowWorkspace_()
{
    // 풀스크린 도크스페이스. 에디터의 베이스가 됨
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
    }

    // 메인 메뉴 바
    {
        if (ImGui::BeginMenuBar())
        {
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
    }

    ImGui::End();
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

void Editor::LoadIcons_()
{
    m_icons[eEditorIcon::Cursor]      = LoadIconFromSvgSource(k_cursorIconSvg);
    m_icons[eEditorIcon::Stop]        = LoadIconFromSvgSource(k_stopIconSvg);
    m_icons[eEditorIcon::Menu]        = LoadIconFromSvgSource(k_menuIconSvg);
    m_icons[eEditorIcon::Play]        = LoadIconFromSvgSource(k_playIconSvg);
    m_icons[eEditorIcon::Rotation]    = LoadIconFromSvgSource(k_rotationIconSvg);
    m_icons[eEditorIcon::Scale]       = LoadIconFromSvgSource(k_scaleIconSvg);
    m_icons[eEditorIcon::Translation] = LoadIconFromSvgSource(k_translationIconSvg);
}

void Editor::UpdateAutoSaveState_(
    const float _deltaSec)
{
    m_configSaveCounterSec += _deltaSec;
    if (m_configSaveCounterSec >= Editor::k_configSaveIntervalSec)
    {
        // save config only in edit m_mode
        if (m_mode == eEditorMode::Edit)
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
    writer.WriteField("vertexLayoutMap", m_vlMap);

    if (const Error err = writer.SaveToFile(m_configPath, eJsonWriteFlags::Pretty))
    {
        JAM_LOG.Warn("Editor::SaveConfig_() - failed to save editor config file. {}.", err.ToErrorMessage());
    }

    JAM_DEBUG_LOG("Editor::SaveConfig_() - saved editor config file.");
}

void Editor::LoadConfigIfExist_()
{
    if (!fs::exists(m_configPath))
    {
        return;
    }

    Expect<JsonReader> reader = JsonReader::ParseFromFile(m_configPath);
    if (!reader)
    {
        return;
    }

    // general
    reader->ReadField("windowShows", m_windowShows);
    reader->ReadField("fontScale", ImGui::GetIO().FontGlobalScale);
    reader->ReadField("renderView", *m_pSceneViewWindow);
    reader->ReadField("fileBrowser", *m_pFileBrowserWindow);
    reader->ReadField("vertexLayoutMap", m_vlMap);
}

Ref<Texture> Editor::LoadIconFromSvgSource(
    const StringView _svgSource) const
{
    SVGImporterInfo desc = {};
    desc.width           = k_iconSize.x;
    desc.height          = k_iconSize.y;

    Expect<SVGImporter> importer = SVGImporter::ImportFromSource(_svgSource, desc);
    if (!importer)
    {
        JAM_UNREACHABLE;
    }

    const CMemoryBlock  mems[] = { importer->GetPixels() };
    const Int32         w      = importer->GetWidth();
    const Int32         h      = importer->GetHeight();
    const TextureHandle texh   = JAM_GFX.CreateTexture2D(w, h, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::None, mems);

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
    m_mode = _mode;
}

eEditorMode Editor::GetMode() const
{
    return m_mode;
}

void Editor::SetFocusingEntity(
    const Entity _entity)
{
    m_focusedEntity = _entity;
}

void Editor::ReleaseFocusingEntity()
{
    m_focusedEntity = Entity::k_invalid;
}

Entity Editor::GetFocusingEntity() const
{
    return m_focusedEntity;
}

float Editor::GetStyleVar(
    const eEditorStyleVar _var) const
{
    return m_style.vars[UnderlyingFrom(_var)];
}

ImVec4 Editor::GetStyleColor(
    const eEditorStyleColor _col) const
{
    return m_style.colors[UnderlyingFrom(_col)];
}

ImU32 Editor::GetStyleColorU32(
    const eEditorStyleColor _col) const
{
    const ImVec4 col = m_style.colors[UnderlyingFrom(_col)];
    return ImGui::ColorConvertFloat4ToU32(col);
}

ImGuiKeyChord Editor::GetStyleShortCut(
    const eEditorShortCut _shortCut) const
{
    return m_style.shortCuts[UnderlyingFrom(_shortCut)];
}

Ref<Texture> Editor::GetIcon(
    const eEditorIcon _icon) const
{
    return m_icons[_icon];
}

VertexLayoutMap& Editor::GetVertexLayoutMap()
{
    return m_vlMap;
}

void Editor::PushDialog(
    const Ref<Dialog>& _pDialog)
{
    m_dialogStack.Push(_pDialog);
}

void Editor::LoadScenByFileDialog()
{
    constexpr FileDialogFilter k_filters[] = {
        { "scene file", Scene::k_extension }
    };

    FileDialog fd { eFileDialogMode::OpenFile };
    fd.SetTitle("load scene");
    fd.SetFilters(k_filters);
    if (fd.Open())
    {
        const Path& path = fd.GetFirstSelectedPath();
        if (!JAM_SCENE_MGR.LoadScene(path))
        {
            ScratchBuffer tmp;
            tmp.Format("failed to load scene file: {}.", path.native());

            MessageDialogInfo desc = {};
            desc.baseInfo.title    = "error";
            desc.msg               = tmp.ToStringView();
            PushDialog(MakeRef<MessageDialog>(desc));
        }
    }
}

void Editor::SaveSceneByFileDialog(
    const Ref<Scene>&           _pScene,
    const Flags<eSaveFileFlags> _flags)
{
    constexpr FileDialogFilter k_filters[] = {
        { "scene file", Scene::k_extension }
    };

    FileDialog fd { eFileDialogMode::SaveFile };
    fd.SetTitle("save scene");
    fd.SetFilters(k_filters);
    if (fd.Open())
    {
        const Path& path = fd.GetFirstSelectedPath();

        JsonWriter writer = {};
        SerializeScene(writer, _pScene);
        if (const Error err = writer.SaveToFile(path, eJsonWriteFlags::Pretty, _flags))
        {
            ScratchBuffer buf;
            buf.Format("failed to save scene file: {}.\n{}", path, err.ToErrorMessage());

            MessageDialogInfo desc = {};
            desc.baseInfo.title    = "error";
            desc.msg               = buf.ToStringView();
            PushDialog(MakeRef<MessageDialog>(desc));
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
        auto onClickConfirm = [_pScene, _entity]
        {
            _pScene->DestroyEntity(_entity);
        };

        ConfirmationDialogInfo desc = {};
        desc.baseInfo.title         = "confirmation";
        desc.msg                    = "the entity has child entities. are you sure you want to destroy it and all its children?";
        desc.OnConfirm              = Bind(std::move(onClickConfirm));
        PushDialog(MakeRef<ConfirmationDialog>(desc));
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
    const Path& _path)
{
    // 만약, 디렉토리이고 비어있지 않다면 삭제 불가 -> 확인 다이얼로그 표시
    if (fs::is_directory(_path) && !fs::is_empty(_path))
    {
        auto onClickConfirm = [this, _path]
        {
            std::error_code ec = {};
            fs::remove_all(_path, ec);
            if (ec)
            {
                ScratchBuffer tmp;
                tmp.Format("failed to remove directory: {}.\n{}", _path, ec.message());

                MessageDialogInfo desc = {};
                desc.baseInfo.title    = "error";
                desc.msg               = tmp.ToStringView();
                PushDialog(MakeRef<MessageDialog>(desc));
            }
        };

        ConfirmationDialogInfo desc = {};
        desc.baseInfo.title         = "confirmation";
        desc.msg                    = "the directory is not empty. are you sure you want to remove it and all its contents?";
        desc.OnConfirm              = Bind(std::move(onClickConfirm));
        PushDialog(MakeRef<ConfirmationDialog>(desc));
        return false;
    }

    // 빈 디렉토리 혹은 파일인 경우 바로 삭제
    std::error_code ec = {};
    fs::remove(_path, ec);
    if (ec)
    {
        ScratchBuffer tmp;
        tmp.Format("failed to remove file: {}.\n{}", _path, ec.message());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    return true;
}

bool Editor::ReplaceFileDir(
    const Path& _srcPath,
    const Path& _newDir)
{
    JAM_ASSERT(fs::is_directory(_newDir), "New directory path is not a directory!");
    return RenameFile(_srcPath, _newDir / _srcPath.filename());
}

bool Editor::ReplaceFileName(
    const Path&      _srcPath,
    const StringView _newFilename)
{
    Path path = _srcPath;
    path.replace_filename(_newFilename.GetPtr());
    return RenameFile(_srcPath, path);
}

bool Editor::RenameFile(
    const Path& _srcPath,
    const Path& _dstPath)
{
    // same path
    if (_srcPath == _dstPath)
    {
        return true;
    }

    // destination file already exists
    if (fs::exists(_dstPath))
    {
        ScratchBuffer tmp;
        tmp.Format("cannot rename file to '{}': file already exists.", _dstPath);

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    // rename
    std::error_code ec = {};
    fs::rename(_srcPath, _dstPath, ec);
    if (ec)
    {
        ScratchBuffer tmp;
        tmp.Format("failed to rename file from '{}' to '{}'.\n{}", _srcPath.native(), _dstPath.native(), ec.message());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    return true;
}

bool Editor::CreateDir(
    const Path& _dirPath)
{
    std::error_code ec = {};
    fs::create_directories(_dirPath, ec);
    if (ec)
    {
        ScratchBuffer tmp;
        tmp.Format("failed to create directory: {}.\n{}", _dirPath.native(), ec.message());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return false;
    }

    return true;
}

bool Editor::CreateNewDir(
    const Path& _parentDir)
{
    JAM_ASSERT(fs::is_directory(_parentDir), "Parent directory path is not a directory!");

    constexpr Int32 k_limit = 1'000;
    for (Int32 i = 0; i < k_limit; ++i)
    {
        ScratchBuffer tmp;
        tmp.Set("new folder");
        if (i > 0)
        {
            tmp.AppendFormat(" ({})", i);
        }

        const Path dirPath = _parentDir / tmp.GetPtr();
        if (!fs::exists(dirPath))
        {
            return CreateDir(dirPath);
        }
    }

    // too many folders named "new folder"
    MessageDialogInfo info = {};
    info.baseInfo.title    = "error";
    info.msg               = "failed to create new folder: too many folders named 'new folder' exist.";
    PushDialog(MakeRef<MessageDialog>(info));
    return false;
}

Optional<Ref<Mesh>> Editor::LoadMesh(
    const Path& _path,
    const bool  _bForceReload)
{
    Expect<Ref<Mesh>> pMesh = JAM_ASSET_MGR.LoadMesh(_path, _bForceReload);
    if (!pMesh)
    {
        ScratchBuffer buf;
        buf.Format("failed to load mesh file: {}. {}.", _path.native(), pMesh.GetError().ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = buf.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return k_nullopt;
    }
    return pMesh.Take();
}

Optional<Ref<Texture>> Editor::LoadTexture(
    const Path& _path,
    const bool  _bForceReload)
{
    Expect<Ref<Texture>> pTexture = JAM_ASSET_MGR.LoadTexture(_path, _bForceReload);
    if (!pTexture)
    {
        ScratchBuffer buf;
        buf.Format("failed to load texture file: {}. {}.", _path.native(), pTexture.GetError().ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = buf.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return k_nullopt;
    }
    return pTexture.Take();
}

Optional<Ref<Material>> Editor::LoadMaterial(
    const Path& _path,
    const bool  _bForceReload)
{
    Expect<Ref<Material>> pMaterial = JAM_ASSET_MGR.LoadMaterial(_path, _bForceReload);
    if (!pMaterial)
    {
        ScratchBuffer buf;
        buf.Format("failed to load material file: {}. {}.", _path.native(), pMaterial.GetError().ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = buf.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return k_nullopt;
    }
    return pMaterial.Take();
}

Optional<Ref<Model>> Editor::LoadModel(
    const Path& _path,
    const bool  _bForceReload)
{
    Expect<Ref<Model>> pModel = JAM_ASSET_MGR.LoadModel(_path, _bForceReload);
    if (!pModel)
    {
        ScratchBuffer buf;
        buf.Format("failed to load model file: {}. {}.", _path.native(), pModel.GetError().ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = buf.ToStringView();
        PushDialog(MakeRef<MessageDialog>(desc));
        return k_nullopt;
    }
    return pModel.Take();
}

Editor::Editor()
    : m_pSceneViewWindow(MakeScoped<Editor_SceneViewWindow>())
    , m_pProfilerWindow(MakeScoped<Editor_ProfilerWindow>())
    , m_pSceneHierarchyWindow(MakeScoped<Editor_SceneHierarchyWindow>())
    , m_pEntityInspectorWindow(MakeScoped<Editor_EntityInspectorWindow>())
    , m_pFileBrowserWindow(MakeScoped<Editor_FileBrowserWindow>())
    , m_pVertexLayoutEditorWindow(MakeScoped<Editor_VertexLayoutEditorWindow>())
{
    m_configPath = JoinedPath(JAM_APP.GetWorkingDirectory(), Editor::k_configFileName);
    SetupDefaultTheme_();
    LoadIcons_();
    LoadConfigIfExist_();
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
    JAM_ASSERT(s_pInstance != nullptr, "Editor instance does not exist!");
    delete s_pInstance;
}

void Editor::Update(const float _deltaSec)
{
    // auto save config
    UpdateAutoSaveState_(_deltaSec);

    // update render view
    m_pSceneViewWindow->OnUpdate(_deltaSec);
}

void Editor::Render() const
{
    m_pSceneViewWindow->OnRender();
}

void Editor::ImGuiRender()
{
    ShowWorkspace_();

    // viewport
    if (bool& bShow = GetWindowShow(eEditorWindowType::SceneView))
    {
        m_pSceneViewWindow->OnImGuiRender(bShow);
    }

    ImGui::BeginDisabled(m_mode == eEditorMode::Play);

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
