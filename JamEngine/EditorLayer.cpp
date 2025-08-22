#include "pch.h"

#include "EditorLayer.h"

#include "Application.h"
#include "AssetUtilities.h"
#include "Components.h"
#include "IModalBox.h"
#include "JsonUtilities.h"
#include "ModalBoxes.h"
#include "STLUtilities.h"

#include <efsw/efsw.hpp>

using namespace jam;

// struct
namespace
{

class EditorFileWatcherListener : public efsw::FileWatchListener
{
public:
    void handleFileAction(efsw::WatchID _watchId, const std::string& _dir, const std::string& _filename, const efsw::Action _action, const std::string _oldFilename) override
    {
        Application& app  = GetApplication();
        fs::path     path = fs::path(_dir) / _filename;

        switch (_action)
        {
            case efsw::Action::Add:
            {
                Log::Debug("FileWatcher: Add: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileAddEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Delete:
            {
                Log::Debug("FileWatcher: Delete: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileRemoveEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Modified:
            {
                Log::Debug("FileWatcher: Modified: {} - {}", _dir, _filename);
                app.SubmitCommand(
                    [&app, path]
                    {
                        FileModifiedEvent e { path };
                        app.DispatchEvent(e);
                    });
            }
            break;

            case efsw::Action::Moved:
            {
                fs::path oldPath = fs::path(_dir) / _oldFilename;
                Log::Debug("FileWatcher: Renamed: {} - {} -> {}", _dir, _oldFilename, _filename);
                app.SubmitCommand(
                    [&app, oldPath, path]
                    {
                        FileRenamedEvent e { oldPath, path };
                        app.DispatchEvent(e);
                    });
            }
            break;
        }
    }
};

}   // namespace

// global variables
namespace
{

Scope<efsw::FileWatcher>         g_fileWatcher         = nullptr;   // file watcher for content browser
Scope<EditorFileWatcherListener> g_fileWatcherListener = nullptr;   // file watcher listener for content browser
efsw::WatchID                    g_fileWatcgerID;                   // watch ID for content browser

}   // namespace

namespace jam
{

EditorLayer::EditorLayer()
    : m_panelShowBit({})
    , m_contentsBrowserPanel(this)
    , m_entityInspectorPanel(this)
    , m_assetInspectorPanel(this)
    , m_sceneHierarchyPanel(this)
    , m_mainMenuBarPanel(this)
{
    // file watcher
    g_fileWatcher         = MakeScope<efsw::FileWatcher>();
    g_fileWatcherListener = MakeScope<EditorFileWatcherListener>();
    g_fileWatcgerID       = g_fileWatcher->addWatch(GetApplication().GetContentsDirectory().string(), g_fileWatcherListener.get(), true);
    g_fileWatcher->watch();

    // 테마 설정
    SetEditorTheme(eEditorTheme::Default);   // 기본 테마로 설정

    // deserilize editor config from file
    LoadFromFileAndDeserialize();
}

EditorLayer::~EditorLayer()
{
    // remove file watcher
    g_fileWatcher->removeWatch(g_fileWatcgerID);

    // serialize and save editor config to file
    SerializeAndSaveToFile();
}

void EditorLayer::OnRender()
{
    // dock space
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

    // main menu bar
    m_mainMenuBarPanel.Show();

    bool& bShowViewport = m_panelShowBit[EnumToInt(eEditorPanel::Viewport)];
    if (bShowViewport)
    {
        m_viewportPanel.Show(&bShowViewport);
    }

    bool& bShowSceneHierarchy = m_panelShowBit[EnumToInt(eEditorPanel::SceneHierarchy)];
    if (bShowSceneHierarchy)
    {
        m_sceneHierarchyPanel.Show(&bShowSceneHierarchy);
    }

    bool& bShowDebugPanel = m_panelShowBit[EnumToInt(eEditorPanel::Debug)];
    if (bShowDebugPanel)
    {
        m_debugPanel.Show(&bShowDebugPanel);
    }

    bool& bShowAssetInspector = m_panelShowBit[EnumToInt(eEditorPanel::AssetInspector)];
    if (bShowAssetInspector)
    {
        m_assetInspectorPanel.Show(&bShowAssetInspector);
    }

    bool& bShowEntityInspector = m_panelShowBit[EnumToInt(eEditorPanel::EntityInspector)];
    if (bShowEntityInspector)
    {
        m_entityInspectorPanel.Show(&bShowEntityInspector);
    }

    bool& bShowContentsBrowser = m_panelShowBit[EnumToInt(eEditorPanel::ContentsBrowser)];
    if (bShowContentsBrowser)
    {
        m_contentsBrowserPanel.Show(&bShowContentsBrowser);
    }

    bool& bShowConsole = m_panelShowBit[EnumToInt(eEditorPanel::Console)];
    if (bShowConsole)
    {
        m_consolePanel.Show(&bShowConsole);
    }

    ShowModalBox_();
}

void EditorLayer::OnEvent(Event& _eventRef)
{
    m_contentsBrowserPanel.OnEvent(_eventRef);
    m_assetInspectorPanel.OnEvent(_eventRef);
}

void EditorLayer::SetPanelVisible(const eEditorPanel _flags, const bool _bEnable)
{
    m_panelShowBit[EnumToInt(_flags)] = _bEnable;
}

void EditorLayer::TogglePanelVisible(const eEditorPanel _flags)
{
    SetPanelVisible(_flags, !IsPanelVisible(_flags));
}

bool EditorLayer::IsPanelVisible(const eEditorPanel _flags) const
{
    return m_panelShowBit[EnumToInt(_flags)];
}

void EditorLayer::OpenModalBoxPanel(Scope<IModalBoxPanel>&& _pModalBoxPanel)
{
    JAM_ASSERT(_pModalBoxPanel, "Message box pointer is null.");
    m_bShowModalBoxSignal = true;
    m_modalBoxPanelStack.emplace_back(std::move(_pModalBoxPanel));   // 모달 박스 스택에 추가
}

void EditorLayer::SerializeAndSaveToFile()
{
    Json json;

    // 패널 표시 여부
    for (eEditorPanel bit: EnumRange<eEditorPanel>())
    {
        json[EnumToString(bit)] = m_panelShowBit[EnumToInt(bit)];
    }

    // 에디터 모드에서의 윈도우 크기
    {
        const Window& window = GetApplication().GetWindow();
        auto [width, height] = window.GetWindowSize();
        json["windowWidth"]  = width;
        json["windowHeight"] = height;
    }

    // 테마 설정
    {
        json["theme"] = m_editorTheme;
    }

    fs::path configPath = GetApplication().GetWorkingDirectory() / k_editorConfigFilename;
    if (!SaveJsonToFile(json, configPath))
    {
        JAM_ERROR("Failed to save editor config to file: {}", configPath.string());
        return;
    }
}

void EditorLayer::LoadFromFileAndDeserialize()
{
    fs::path configPath = GetApplication().GetWorkingDirectory() / k_editorConfigFilename;
    if (!fs::exists(configPath))   // 파일이 존재하지 않으면 종료
    {
        return;
    }

    // json 파일 로드
    auto [json, bResult] = LoadJsonFromFile(configPath);
    if (bResult == false)   // 파일 로드 실패
    {
        JAM_ERROR("Failed to load editor config from file: {}", configPath.string());
        return;
    }

    // 패널 표시 여부
    for (eEditorPanel bit: EnumRange<eEditorPanel>())
    {
        m_panelShowBit[EnumToInt(bit)] = GetJsonValueOrDefault(json, EnumToString(bit), false);
    }

    // 에디터 모드에서의 윈도우 크기
    {
        const Window& window       = GetApplication().GetWindow();
        auto [oriWidth, oriHeight] = window.GetWindowSize();
        Int32 width                = GetJsonValueOrDefault(json, "windowWidth", oriWidth);
        Int32 height               = GetJsonValueOrDefault(json, "windowHeight", oriHeight);
        window.ResizeWindow(width, height);
    }

    // 에디터 테마 설정
    {
        m_editorTheme = GetJsonValueOrDefault(json, "theme", eEditorTheme::Default);
        SetEditorTheme(m_editorTheme);   // 테마 설정
    }
}

void EditorLayer::LoadAsset(AssetManager& _assetMgrRef, eAssetType _type, const fs::path& _path)
{
    JAM_ASSERT(IsValidEnum(_type), "Invalid asset type: {}", EnumToInt(_type));

    if (!IsCompatibleFromPath(_type, _path))   // 에셋 타입과 경로가 호환되는지 확인
    {
        JAM_ERROR("Path '{}' is not compatible with asset type '{}'.", _path.string(), EnumToString(_type));

        constexpr const char* const k_title = "asset load failed";
        std::string                 msg     = std::format("path '{}' is not compatible with asset type '{}'.", _path.string(), EnumToString(_type));
        OpenModalBoxPanel(MakeScope<OkModalBox>(k_title, msg.c_str()));   // 모달 박스 열기
        return;
    }

    if (_assetMgrRef.Contain(_type, _path))   // 만약 이미 존재한다면 덮어쓸지 확인
    {
        constexpr const char* const k_title = "asset already exists";
        constexpr const char* const k_msg   = "asset already exists at this path.\nDo you want to overwrite it?";

        const auto OnYesCallback = [&_assetMgrRef, _type, _path]
        {
            _assetMgrRef.Load(_type, _path);
        };

        // yes or no 모달 박스 생성
        OpenModalBoxPanel(MakeScope<YesOrNoModalBox>(k_title, k_msg, OnYesCallback));   // 모달 박스 열기
    }
    else   // 새로운 에셋이라면 그냥 로드
    {
        auto [_, bResult] = _assetMgrRef.Load(_type, _path);   // 에셋 로드 시도
        if (bResult == false)                                  // 실패시
        {
            constexpr const char* const k_title = "asset load failed";
            constexpr const char* const k_msg   = "failed to load asset from the specified path.";

            // 실패시 메시지 처리
            OpenModalBoxPanel(MakeScope<OkModalBox>(k_title, k_msg));   // 모달 박스 열기
        }
    }
}

void EditorLayer::ReloadAsset(AssetManager& _assetMgrRef, const eAssetType _type, const fs::path& _path)
{
    JAM_ASSERT(IsCompatibleFromPath(_type, _path), "Path '{}' is not compatible with asset type '{}'.", _path.string(), EnumToString(_type));

    auto [_, bResult] = _assetMgrRef.Load(_type, _path);   // 에셋 리로드 시도
    if (bResult == false)                                  // 실패시
    {
        constexpr const char* const k_title = "asset load failed";
        constexpr const char* const k_msg   = "failed to load asset from the specified path.";
        OpenModalBoxPanel(MakeScope<OkModalBox>(k_title, k_msg));   // 모달 박스 열기
    }
}

void EditorLayer::UnloadAsset(AssetManager& _assetMgrRef, const eAssetType _type, const fs::path& _path)
{
    JAM_ASSERT(IsCompatibleFromPath(_type, _path), "Path '{}' is not compatible with asset type '{}'.", _path.string(), EnumToString(_type));

    if (!_assetMgrRef.Unload(_type, _path))
    {
        constexpr const char* const k_title = "asset unload failed";
        constexpr const char* const k_msg   = "failed to unload asset from the specified path.";
        OpenModalBoxPanel(MakeScope<OkModalBox>(k_title, k_msg));   // 모달 박스 열기
    }
}

void EditorLayer::DestroyEntity(const Entity& _entity)
{
    bool bIsEditingEntity = m_editEntity == _entity;   // 현재 편집중인 엔티티와 동일한지 확인
    _entity.Destroy();                                 // 엔티티 삭제
    if (bIsEditingEntity)                              // 만약 현재 편집중인 엔티티였다면
    {
        SetEditEntity(Entity::s_null);   // 편집중인 엔티티를 초기화
    }
}

void EditorLayer::CloneEntity(const Entity& _entity)
{
    JAM_ASSERT(_entity.IsValid(), "Entity is not valid.");
    SetEditEntity(_entity.Clone());   // 복제된 엔티티를 편집 엔티티로 설정
}

void EditorLayer::SaveScene(Scene* _pScene, const std::optional<fs::path>& _path)
{
    JAM_ASSERT(_pScene, "Scene pointer cannot be null.");

    constexpr const char* const k_titles[2] = {
        "save scene succeeded",
        "save scene failed"
    };   // 모달 박스 제목

    constexpr const char* const k_msgs[2] = {
        "scene has been saved successfully.",
        "failed to save scene."
    };   // 모달 박스 메시지

    bool bResult = _pScene->Save(_path);                                        // 기본 경로로 씬 저장
    int  index   = bResult ? 0 : 1;                                             // 성공 여부에 따라 인덱스 설정
    OpenModalBoxPanel(MakeScope<OkModalBox>(k_titles[index], k_msgs[index]));   // 모달 박스 열기
}

void EditorLayer::LoadScene(Scene* _pScene, const std::optional<fs::path>& _path)
{
    JAM_ASSERT(_pScene, "Scene pointer cannot be null.");

    constexpr const char* const k_titles[2] = {
        "load scene succeeded",
        "load scene failed"
    };   // 모달 박스 제목

    constexpr const char* const k_msgs[2] = {
        "scene has been loaded successfully.",
        "failed to load scene."
    };   // 모달 박스 메시지

    bool bResult = _pScene->Load(_path);                                        // 씬 로드 시도
    int  index   = bResult ? 0 : 1;                                             // 성공 여부에 따라 인덱스 설정
    OpenModalBoxPanel(MakeScope<OkModalBox>(k_titles[index], k_msgs[index]));   // 모달 박스 열기
}

void EditorLayer::ShowModalBox_()
{
    if (m_modalBoxPanelStack.empty() == false)
    {
        Scope<IModalBoxPanel>& pModalBox = m_modalBoxPanelStack.back();    // 현재 모달 박스
        const char*            title     = pModalBox->GetTitle().data();   // 모달 박스 제목

        if (m_bShowModalBoxSignal)   // 만약 모달 박스가 열려야 한다면
        {
            ImGui::OpenPopup(title);
            m_bShowModalBoxSignal = false;   // 모달 박스가 열렸으니 신호를 초기화
        }

        bool           bIsOpen       = true;   // 팝업은 기본적으로 열려있음
        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(pMainViewport->GetCenter(), ImGuiCond_Appearing, { 0.5f, 0.5f });   // 등장시에 중앙에 위치하도록 설정
        if (ImGui::BeginPopupModal(title, &bIsOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // 모달 내용 그리기
            pModalBox->Show();

            // 윈도우의 x 버튼을 눌렀다면..
            if (!bIsOpen)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // 팝업이 아예 닫혀버렸다면 (X 버튼 or 내부 Close)
        if (ImGui::IsPopupOpen(title) == false)
        {
            Erase(m_modalBoxPanelStack, pModalBox);   // 모달 박스 스택에서 제거
            m_bShowModalBoxSignal = true;             // 다음 모달이 있으면 자동으로 열리도록
        }
    }
}

void EditorLayer::SetEditorTheme(const eEditorTheme _theme)
{
    m_editorTheme = _theme;   // 현재 테마 업데이트
    ResetImGuiTheme_();       // 테마 초기화
    switch (m_editorTheme)
    {
        case eEditorTheme::Dark:
            ImGui::StyleColorsDark();
            break;
        case eEditorTheme::Light:
            ImGui::StyleColorsLight();
            break;
        case eEditorTheme::JamEngine1:
            SetEditorThemeByJamEngine1_();
            break;
        case eEditorTheme::JamEngine2:
            SetEditorThemeByJamEngine2_();
            break;
        case eEditorTheme::Moonlight:
            SetEditorThemeByMoonlight_();
            break;
        case eEditorTheme::Material:
            SetEditorThemeByMaterial_();
            break;
        case eEditorTheme::Adobe:
            SetEditorThemeByAdobe_();
            break;
        case eEditorTheme::Unreal:
            SetEditorThemeByUnreal_();
            break;
        case eEditorTheme::VisualStudio:
            SetEditorThemeByVisualStudio_();
            break;
        default:
            JAM_ASSERT(false, "Unknown editor theme: {}", EnumToString(m_editorTheme));
    }
}

void EditorLayer::SetEditorThemeByJamEngine1_() const
{
    ImGuiStyle& style  = ImGui::GetStyle();
    ImVec4*     colors = style.Colors;

    // Primary background
    colors[ImGuiCol_WindowBg]  = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);   // #131318
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);   // #131318

    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    // Headers
    colors[ImGuiCol_Header]        = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

    // Frame BG
    colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

    // Tabs
    colors[ImGuiCol_Tab]                = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

    // Title
    colors[ImGuiCol_TitleBg]          = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    // Borders
    colors[ImGuiCol_Border]       = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text
    colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    // Highlights
    colors[ImGuiCol_CheckMark]         = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]        = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

    // Style tweaks
    style.WindowRounding    = 5.0f;
    style.FrameRounding     = 5.0f;
    style.GrabRounding      = 5.0f;
    style.TabRounding       = 5.0f;
    style.PopupRounding     = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.WindowPadding     = ImVec2(10, 10);
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.PopupBorderSize   = 0.f;
}

void EditorLayer::SetEditorThemeByMoonlight_() const
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 1.0f;
    style.WindowPadding            = ImVec2(12.0f, 12.0f);
    style.WindowRounding           = 11.5f;
    style.WindowBorderSize         = 0.0f;
    style.WindowMinSize            = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding            = 0.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 0.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(20.0f, 3.400000095367432f);
    style.FrameRounding            = 11.89999961853027f;
    style.FrameBorderSize          = 0.0f;
    style.ItemSpacing              = ImVec2(4.300000190734863f, 5.5f);
    style.ItemInnerSpacing         = ImVec2(7.099999904632568f, 1.799999952316284f);
    style.CellPadding              = ImVec2(12.10000038146973f, 9.199999809265137f);
    style.IndentSpacing            = 0.0f;
    style.ColumnsMinSpacing        = 4.900000095367432f;
    style.ScrollbarSize            = 11.60000038146973f;
    style.ScrollbarRounding        = 15.89999961853027f;
    style.GrabMinSize              = 3.700000047683716f;
    style.GrabRounding             = 20.0f;
    style.TabRounding              = 0.0f;
    style.TabBorderSize            = 0.0f;
    style.ColorButtonPosition      = ImGuiDir_Right;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.2745098173618317f, 0.317647069692611f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.09250493347644806f, 0.100297249853611f, 0.1158798336982727f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.1120669096708298f, 0.1262156516313553f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.971993625164032f, 1.0f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 0.7953379154205322f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1821731775999069f, 0.1897992044687271f, 0.1974248886108398f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.1545050293207169f, 0.1545048952102661f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.1414651423692703f, 0.1629818230867386f, 0.2060086131095886f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1072951927781105f, 0.107295036315918f, 0.1072961091995239f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.1293079704046249f, 0.1479243338108063f, 0.1931330561637878f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.1459212601184845f, 0.1459220051765442f, 0.1459227204322815f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.999999463558197f, 1.0f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1249424293637276f, 0.2735691666603088f, 0.5708154439926147f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8841201663017273f, 0.7941429018974304f, 0.5615870356559753f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.9570815563201904f, 0.9570719599723816f, 0.9570761322975159f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.9356134533882141f, 0.9356129765510559f, 0.9356223344802856f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}

void EditorLayer::SetEditorThemeByJamEngine2_() const
{
    ImGuiStyle& style  = ImGui::GetStyle();
    ImVec4*     colors = style.Colors;

    colors[ImGuiCol_Text]                      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]              = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_WindowBg]                  = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_ChildBg]                   = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_PopupBg]                   = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
    colors[ImGuiCol_Border]                    = ImVec4(0.04f, 0.04f, 0.04f, 0.99f);
    colors[ImGuiCol_BorderShadow]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
    colors[ImGuiCol_FrameBgActive]             = ImVec4(0.03f, 0.03f, 0.04f, 0.67f);
    colors[ImGuiCol_TitleBg]                   = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
    colors[ImGuiCol_TitleBgActive]             = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(0.18f, 0.17f, 0.17f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_CheckMark]                 = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
    colors[ImGuiCol_SliderGrab]                = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.43f, 0.90f, 0.11f, 1.00f);
    colors[ImGuiCol_Button]                    = ImVec4(0.21f, 0.22f, 0.23f, 0.40f);
    colors[ImGuiCol_ButtonHovered]             = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
    colors[ImGuiCol_ButtonActive]              = ImVec4(0.54f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Header]                    = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_HeaderHovered]             = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
    colors[ImGuiCol_HeaderActive]              = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    colors[ImGuiCol_Separator]                 = ImVec4(0.16f, 0.16f, 0.16f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]           = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]                = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]                = ImVec4(0.23f, 0.23f, 0.24f, 0.80f);
    colors[ImGuiCol_Tab]                       = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabSelected]               = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.13f, 0.78f, 0.07f, 1.00f);
    colors[ImGuiCol_TabDimmed]                 = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.10f, 0.60f, 0.12f, 1.00f);
    colors[ImGuiCol_DockingPreview]            = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]            = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]                 = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.14f, 0.87f, 0.05f, 1.00f);
    colors[ImGuiCol_PlotHistogram]             = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(0.23f, 0.78f, 0.02f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]          = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]             = ImVec4(0.46f, 0.47f, 0.46f, 0.06f);
    colors[ImGuiCol_TextLink]                  = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]            = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]            = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]                 = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.78f, 0.69f, 0.69f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowRounding           = 4.0f;
    style.FrameRounding            = 4.0f;
    style.GrabRounding             = 3.0f;
    style.PopupRounding            = 4.0f;
    style.TabRounding              = 4.0f;
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ScrollbarSize            = 10.0f;
    style.GrabMinSize              = 10.0f;
    style.DockingSeparatorSize     = 1.0f;
    style.SeparatorTextBorderSize  = 2.0f;
}

void EditorLayer::SetEditorThemeByMaterial_() const
{
    // Material Flat style by ImJC1C from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 0.5f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 0.0f;
    style.WindowBorderSize         = 1.0f;
    style.WindowMinSize            = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding            = 0.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 0.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(4.0f, 3.0f);
    style.FrameRounding            = 0.0f;
    style.FrameBorderSize          = 0.0f;
    style.ItemSpacing              = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
    style.CellPadding              = ImVec2(4.0f, 2.0f);
    style.IndentSpacing            = 21.0f;
    style.ColumnsMinSpacing        = 6.0f;
    style.ScrollbarSize            = 14.0f;
    style.ScrollbarRounding        = 0.0f;
    style.GrabMinSize              = 10.0f;
    style.GrabRounding             = 0.0f;
    style.TabRounding              = 0.0f;
    style.TabBorderSize            = 0.0f;
    style.ColorButtonPosition      = ImGuiDir_Left;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.8313725590705872f, 0.8470588326454163f, 0.8784313797950745f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.8313725590705872f, 0.8470588326454163f, 0.8784313797950745f, 0.501960813999176f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1725490242242813f, 0.1921568661928177f, 0.2352941185235977f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.0f, 0.0f, 0.0f, 0.1587982773780823f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.1725490242242813f, 0.1921568661928177f, 0.2352941185235977f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.501960813999176f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 0.250980406999588f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.5333333611488342f, 0.5333333611488342f, 0.5333333611488342f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.3333333432674408f, 0.3333333432674408f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.239215686917305f, 0.5215686559677124f, 0.8784313797950745f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.1529411822557449f, 0.1725490242242813f, 0.2117647081613541f, 0.501960813999176f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1529411822557449f, 0.1725490242242813f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.1529411822557449f, 0.1725490242242813f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 0.250980406999588f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 0.250980406999588f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.1529411822557449f, 0.1725490242242813f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 0.250980406999588f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.1529411822557449f, 0.1725490242242813f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.3098039329051971f, 0.6235294342041016f, 0.9333333373069763f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 0.5021458864212036f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.03862661123275757f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 0.7529411911964417f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
}

void EditorLayer::SetEditorThemeByAdobe_() const
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 0.6000000238418579f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 4.0f;
    style.WindowBorderSize         = 1.0f;
    style.WindowMinSize            = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding            = 4.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 2.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(4.0f, 3.0f);
    style.FrameRounding            = 2.0f;
    style.FrameBorderSize          = 1.0f;
    style.ItemSpacing              = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
    style.CellPadding              = ImVec2(4.0f, 2.0f);
    style.IndentSpacing            = 21.0f;
    style.ColumnsMinSpacing        = 6.0f;
    style.ScrollbarSize            = 13.0f;
    style.ScrollbarRounding        = 12.0f;
    style.GrabMinSize              = 7.0f;
    style.GrabRounding             = 0.0f;
    style.TabRounding              = 0.0f;
    style.TabBorderSize            = 1.0f;
    style.ColorButtonPosition      = ImGuiDir_Right;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.2745098173618317f, 0.2745098173618317f, 0.2745098173618317f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.2980392277240753f, 0.2980392277240753f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.0f, 1.0f, 1.0f, 0.3910000026226044f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.0f, 1.0f, 1.0f, 0.6700000166893005f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.5843137502670288f, 0.5843137502670288f, 0.5843137502670288f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
}

void EditorLayer::SetEditorThemeByUnreal_() const
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 0.6000000238418579f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 0.0f;
    style.WindowBorderSize         = 1.0f;
    style.WindowMinSize            = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding            = 0.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 0.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(4.0f, 3.0f);
    style.FrameRounding            = 0.0f;
    style.FrameBorderSize          = 0.0f;
    style.ItemSpacing              = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
    style.CellPadding              = ImVec2(4.0f, 2.0f);
    style.IndentSpacing            = 21.0f;
    style.ColumnsMinSpacing        = 6.0f;
    style.ScrollbarSize            = 14.0f;
    style.ScrollbarRounding        = 9.0f;
    style.GrabMinSize              = 10.0f;
    style.GrabRounding             = 0.0f;
    style.TabRounding              = 4.0f;
    style.TabBorderSize            = 0.0f;
    style.ColorButtonPosition      = ImGuiDir_Right;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.2000000029802322f, 0.2078431397676468f, 0.2196078449487686f, 0.5400000214576721f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 0.6700000166893005f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.2862745225429535f, 0.2862745225429535f, 0.2862745225429535f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.8588235378265381f, 0.8588235378265381f, 0.8588235378265381f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.4000000059604645f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.4588235318660736f, 0.4666666686534882f, 0.47843137383461f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.4196078479290009f, 0.4196078479290009f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.3100000023841858f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.800000011920929f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.47843137383461f, 0.4980392158031464f, 0.5176470875740051f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.7176470756530762f, 0.7176470756530762f, 0.7176470756530762f, 0.7799999713897705f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.9098039269447327f, 0.9098039269447327f, 0.9098039269447327f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.8078431487083435f, 0.8078431487083435f, 0.8078431487083435f, 0.6700000166893005f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.4588235318660736f, 0.4588235318660736f, 0.4588235318660736f, 0.949999988079071f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.729411780834198f, 0.6000000238418579f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.8666666746139526f, 0.8666666746139526f, 0.8666666746139526f, 0.3499999940395355f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

void EditorLayer::SetEditorThemeByVisualStudio_() const
{
    // Visual Studio style by MomoDeve from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                    = 1.0f;
    style.DisabledAlpha            = 0.6000000238418579f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 0.0f;
    style.WindowBorderSize         = 1.0f;
    style.WindowMinSize            = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign         = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding            = 0.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupRounding            = 0.0f;
    style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(4.0f, 3.0f);
    style.FrameRounding            = 0.0f;
    style.FrameBorderSize          = 0.0f;
    style.ItemSpacing              = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 4.0f);
    style.CellPadding              = ImVec2(4.0f, 2.0f);
    style.IndentSpacing            = 21.0f;
    style.ColumnsMinSpacing        = 6.0f;
    style.ScrollbarSize            = 14.0f;
    style.ScrollbarRounding        = 0.0f;
    style.GrabMinSize              = 10.0f;
    style.GrabRounding             = 0.0f;
    style.TabRounding              = 0.0f;
    style.TabBorderSize            = 0.0f;
    style.ColorButtonPosition      = ImGuiDir_Right;
    style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
}

void EditorLayer::ResetImGuiTheme_() const
{
    ImGuiStyle defaultStyle;
    ImGui::GetStyle() = defaultStyle;   // 기본 스타일로 초기화
}

void EditorLayer::SetEditEntity(const Entity& _entity)
{
    m_editEntity = _entity;
}

Entity EditorLayer::GetEditEntity() const
{
    return m_editEntity;
}

}   // namespace jam