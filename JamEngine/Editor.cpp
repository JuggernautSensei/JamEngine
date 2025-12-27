#include "pch.h"

#include "Editor.h"

#include "Application.h"
#include "Editor_EntityInspector.h"
#include "Editor_FileBrowser.h"
#include "Editor_Profiler.h"
#include "Editor_SceneHierarchy.h"
#include "Editor_SceneView.h"
#include "Editor_VertexLayoutEditor.h"
#include "Entity.h"
#include "Vendor/imgui/ImGuizmo.h"
#include "EditorCommandManager.h"
#include "DialogStack.h"

namespace jam
{

struct EditorContext
{
    void OnSceneChanged(SceneChangedEvent& _event) const;
    void OnEntityDestroyed(const EntityDestroyedEvent& _event) const;

    void DrawFileMenu() const;
    void DrawWindowMenu();
    void DrawOptionsMenu() const;

    void ShowWorkspace();
    void SetupDefaultTheme() const;
    void SaveConfig() const;
    void LoadConfig() const;

    [[nodiscard]] bool& GetWindowShow(eEditorWindowType _type);

    // windows
    Editor_SceneView          renderViewWindow;
    Editor_Profiler           profilerWindow;
    Editor_SceneHierarchy     sceneHierarchyWindow;
    Editor_EntityInspector    entityInspectorWindow;
    Editor_FileBrowser        fileBrowserWindow;
    Editor_VertexLayoutEditor vertexLayoutEditorWindow;

    Array<bool, NumEnums<eEditorWindowType>()> windowShows = {};

    // data
    EditorCommandManager commandManager;
    DialogStack          dialogStack;

    Entity              focusedEntity = Entity::s_null;
    eEditorMode         mode          = eEditorMode::Edit;
    EditorStyle         style         = {};
    VertexLayoutStorage vlStorage     = {};

    fs::path configPath           = {};
    float    configSaveCounterSec = 0.f;

    constexpr static float       k_configSaveIntervalSec = 10.f;   // save config every 10 seconds
    constexpr static const char* k_configFileName        = "EditorConfig.json";

}* g_pEdtCtx;

void EditorContext::OnSceneChanged(SceneChangedEvent& _event) const
{
    g_pEdtCtx->focusedEntity = Entity::s_null;
}

void EditorContext::OnEntityDestroyed(const EntityDestroyedEvent& _event) const
{
    if (g_pEdtCtx->focusedEntity.GetUID() == _event.GetEntityUID())
    {
        g_pEdtCtx->focusedEntity = Entity::s_null;
    }
}

void EditorContext::DrawFileMenu() const
{
    if (ImGui::MenuItem("exit"))
        Application::GetInstance().Quit();
}

void EditorContext::DrawWindowMenu()
{
    ImGui::MenuItem("scene view", nullptr, &GetWindowShow(eEditorWindowType::SceneView));
    ImGui::MenuItem("scene hierarchy", nullptr, &GetWindowShow(eEditorWindowType::SceneHierarchy));
    ImGui::MenuItem("entity inspector", nullptr, &GetWindowShow(eEditorWindowType::EntityInspector));
    ImGui::MenuItem("file browser", nullptr, &GetWindowShow(eEditorWindowType::FileBrowser));
    ImGui::MenuItem("profiler", nullptr, &GetWindowShow(eEditorWindowType::Profiler));
    ImGui::MenuItem("vertex layout editor", nullptr, &GetWindowShow(eEditorWindowType::VertexLayoutEditor));
}

void EditorContext::DrawOptionsMenu() const
{
    if (ImGui::BeginMenu("font scale"))
    {
        Widget_DragPercent("##FontScale", ImGui::GetIO().FontGlobalScale, 1.f, 50.f, 200.f);
        ImGui::EndMenu();
    }
}

void EditorContext::ShowWorkspace()
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
        const bool bOpen = ImGui::Begin("Workspace", nullptr, k_flags);
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
                DrawFileMenu();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("window"))
            {
                DrawWindowMenu();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("options"))
            {
                DrawOptionsMenu();
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    ImGui::End();
}

void EditorContext::SetupDefaultTheme() const
{
    // imgui style
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

    // imguizmo style
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

void EditorContext::SaveConfig() const
{
    // Json json;
    // json["windowShows"] = g_pEdtCtx->windowShows;
    // json["fontScale"]   = ImGui::GetIO().FontGlobalScale;
    // json["viewport"]    = g_pEdtCtx->renderViewWindow.OnSerialize();
    // json["fileBrowser"] = g_pEdtCtx->fileBrowserWindow.OnSerialize();
    //
    // const fs::path path = g_pEdtCtx->CreateConfigFilePath();
    // if (SaveJsonFile(path, json))
    //{
    //     JAM_DEBUG_LOG("editor config saved.");
    // }
    // else
    //{
    //     JAM_ASSERT(0, "failed to save editor config file.");
    // }

    //   Json       json;
    //   JsonWriter writer { json };
    //   writer.WriteField("windowShows", g_pEdtCtx->windowShows);
    //   writer.WriteField("fontScale", ImGui::GetIO().FontGlobalScale);
}

void EditorContext::LoadConfig() const
{
    //    const fs::path path = g_pEdtCtx->CreateConfigFilePath();
    //    if (!fs::exists(path))   // no config file
    //    {
    //        return;
    //    }
    //
    //    const Optional<Json> opt = LoadJsonFile(path);
    //    if (!opt)
    //    {
    //        // discard if failed to load
    //        return;
    //    }
    //
    //    // json
    //    const Json& json = opt.value();
    //
    //    try
    //    {
    //        // font scale
    //        ImGuiIO& io        = ImGui::GetIO();
    //        io.FontGlobalScale = json.value("fontScale", io.FontGlobalScale);
    //
    //        // window shows
    //        g_pEdtCtx->windowShows = json.value("windowShows", g_pEdtCtx->windowShows);
    //
    //        // vertex layout
    //        if (const auto it = json.find("vertexLayoutStorage"); it != json.end())
    //        {
    //            const Json& vlJson   = *it;
    //            g_pEdtCtx->vlStorage = DeserializeVertexLayoutStorage(vlJson);
    //        }
    //
    //        // viewport
    //        if (const auto it = json.find("viewport"); it != json.end())
    //        {
    //            const Json& vpJson = *it;
    //            g_pEdtCtx->renderViewWindow.OnDeserialize(vpJson);
    //        }
    //
    //        // file browser
    //        if (const auto it = json.find("fileBrowser"); it != json.end())
    //        {
    //            const Json& fileBrowserJson = *it;
    //            g_pEdtCtx->fileBrowserWindow.OnDeserialize(fileBrowserJson);
    //        }
    //    }
    //    catch (const std::exception& e)
    //    {
    //        // not a critical error, just discard
    //        Log::Warn("failed to parse editor config file: {}", e.what());
    //    }
}

bool& EditorContext::GetWindowShow(
    const eEditorWindowType _type)
{
    return windowShows[IntFrom(_type)];
}

}   // namespace jam

namespace jam
{

void Editor::Init()
{
    JAM_ASSERT(g_pEdtCtx == nullptr, "Editor::Load() called more than once without Shutdown().");
    g_pEdtCtx = JAM_NEW(EditorContext);

    Application& app = Application::GetInstance();

    g_pEdtCtx->configPath = app.JoinWorkingDirectory(EditorContext::k_configFileName);
    g_pEdtCtx->fileBrowserWindow.Init();
    g_pEdtCtx->renderViewWindow.Init();
    g_pEdtCtx->SetupDefaultTheme();
    g_pEdtCtx->LoadConfig();
}

void Editor::Shutdown()
{
    JAM_ASSERT(g_pEdtCtx, "Editor::Shutdown() called without Load().");

    g_pEdtCtx->SaveConfig();
    g_pEdtCtx->fileBrowserWindow.Shutdown();
    g_pEdtCtx->renderViewWindow.Shutdown();
    JAM_DELETE(g_pEdtCtx);
}

void Editor::Update(const float _deltaSec)
{
    // auto save config
    UpdateAutoSaveConfig_(_deltaSec);

    // update render view
    g_pEdtCtx->renderViewWindow.OnUpdate(_deltaSec);
}

void Editor::Render()
{
    g_pEdtCtx->renderViewWindow.OnRender();
}

void Editor::ImGuiRender()
{
    g_pEdtCtx->ShowWorkspace();

    // viewport
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::SceneView))
    {
        g_pEdtCtx->renderViewWindow.OnImGuiRender(bShow);
    }

    ImGui::BeginDisabled(g_pEdtCtx->mode == eEditorMode::Play);

    // profiler
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::Profiler))
    {
        g_pEdtCtx->profilerWindow.OnImGuiRender(bShow);
    }

    // scene hierarchy
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::SceneHierarchy))
    {
        g_pEdtCtx->sceneHierarchyWindow.OnImGuiRender(bShow);
    }

    // entity inspector
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::EntityInspector))
    {
        g_pEdtCtx->entityInspectorWindow.OnImGuiRender(bShow);
    }

    // file system browser
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::FileBrowser))
    {
        g_pEdtCtx->fileBrowserWindow.OnImGuiRender(bShow);
    }

    // vertex vertexLayout manager
    if (bool& bShow = g_pEdtCtx->GetWindowShow(eEditorWindowType::VertexLayoutEditor))
    {
        g_pEdtCtx->vertexLayoutEditorWindow.OnImGuiRender(bShow);
    }

    ImGui::EndDisabled();

    // show dialog
    g_pEdtCtx->dialogStack.Show();
}

void Editor::OnEvent(Event& _event)
{
    HandleEvent<SceneChangedEvent>(_event, g_pEdtCtx, &EditorContext::OnSceneChanged);
    HandleEvent<EntityDestroyedEvent>(_event, g_pEdtCtx, &EditorContext::OnEntityDestroyed);
    g_pEdtCtx->fileBrowserWindow.OnEvent(_event);
    g_pEdtCtx->renderViewWindow.OnEvent(_event);
}

void Editor::SetMode(
    const eEditorMode _mode)
{
    g_pEdtCtx->mode = _mode;
}

eEditorMode Editor::GetMode()
{
    return g_pEdtCtx->mode;
}

void Editor::SetFocusedEntity(
    const Entity _entity)
{
    g_pEdtCtx->focusedEntity = _entity;
}

Entity Editor::GetFocusedEntity()
{
    return g_pEdtCtx->focusedEntity;
}

VertexLayoutStorage& Editor::GetVertexLayoutStorage()
{
    return g_pEdtCtx->vlStorage;
}

EditorStyle& Editor::GetStyle()
{
    return g_pEdtCtx->style;
}

ImVec4 Editor::GetStyleColor(
    const eEditorStyleColor _col)
{
    return g_pEdtCtx->style.colors[IntFrom(_col)];
}

ImU32 Editor::GetStyleColorU32(
    const eEditorStyleColor _col)
{
    const ImVec4 col = g_pEdtCtx->style.colors[IntFrom(_col)];
    return ImGui::ColorConvertFloat4ToU32(col);
}

void Editor::ExecuteCommand(
    Scoped<IEditorCommand>&& _command)
{
    g_pEdtCtx->commandManager.Execute(std::move(_command));
}

void Editor::UndoCommand()
{
    g_pEdtCtx->commandManager.Undo();
}

void Editor::RedoCommand()
{
    g_pEdtCtx->commandManager.Redo();
}

bool Editor::CanUndoCommand()
{
    return g_pEdtCtx->commandManager.CanUndo();
}

bool Editor::CanRedoCommand()
{
    return g_pEdtCtx->commandManager.CanRedo();
}

void Editor::PushDialog(
    Scoped<Dialog>&& _pDialog)
{
    g_pEdtCtx->dialogStack.Push(std::move(_pDialog));
}

void Editor::UpdateAutoSaveConfig_(const float _deltaSec)
{
    g_pEdtCtx->configSaveCounterSec += _deltaSec;
    if (g_pEdtCtx->configSaveCounterSec >= EditorContext::k_configSaveIntervalSec)
    {
        // save config only in edit mode
        if (g_pEdtCtx->mode == eEditorMode::Edit)
        {
            g_pEdtCtx->SaveConfig();
            g_pEdtCtx->configSaveCounterSec = 0.f;
        }
    }
}

}   // namespace jam
