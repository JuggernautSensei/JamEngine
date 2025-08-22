#pragma once
#include "AssetInspectorPanel.h"
#include "ConsolePanel.h"
#include "ContentsBrowserPanel.h"
#include "DebugPanel.h"
#include "Entity.h"
#include "EntityInspectorPanel.h"
#include "EnumUtilities.h"
#include "ILayer.h"
#include "MainMenuBarPanel.h"
#include "SceneHierarchyPanel.h"
#include "ViewportPanel.h"

namespace jam
{

class BackBufferCleanupEvent;
class WindowResizeEvent;
class IModalBoxPanel;

enum class eEditorPanel : Int32
{
    Debug,
    Viewport,
    SceneHierarchy,
    AssetInspector,
    EntityInspector,
    ContentsBrowser,
    Console,
};

enum class eEditorTheme
{
    Dark,
    Light,
    JamEngine1,
    JamEngine2,
    Moonlight,
    Material,
    Adobe,
    Unreal,
    VisualStudio,

    Default = JamEngine1,
};

class EditorLayer : public ILayer
{
public:
    EditorLayer();
    ~EditorLayer() override;

    EditorLayer(const EditorLayer&)                = delete;
    EditorLayer& operator=(const EditorLayer&)     = delete;
    EditorLayer(EditorLayer&&) noexcept            = default;
    EditorLayer& operator=(EditorLayer&&) noexcept = default;

    // ILayer 인터페이스
    void      OnRender() override;
    void      OnEvent(Event& _eventRef) override;
    NODISCARD std::string_view GetName() const override { return NameOf<EditorLayer>(); }
    NODISCARD UInt32           GetHash() const override { return HashOf<EditorLayer>(); }

    // 테마 설정
    void                   SetEditorTheme(eEditorTheme _theme);
    NODISCARD eEditorTheme GetEditorTheme() const { return m_editorTheme; }

    // 패널 관련
    void           SetPanelVisible(eEditorPanel _flags, bool _bEnable);
    void           TogglePanelVisible(eEditorPanel _flags);
    NODISCARD bool IsPanelVisible(eEditorPanel _flags) const;

    // 모달 윈도우 형태로 메시지 박스를 띄우는 함수
    void OpenModalBoxPanel(Scope<IModalBoxPanel>&& _pModalBoxPanel);

    // 편집 중인 엔터티 관련
    void             SetEditEntity(const Entity& _entity);
    NODISCARD Entity GetEditEntity() const;

    // 직렬화 역직렬화
    void SerializeAndSaveToFile();
    void LoadFromFileAndDeserialize();

    // 에셋 관련 유틸리티: gui에서 사용하는 유틸리티 함수. editor와의 상호작용 포함
    void LoadAsset(AssetManager& _assetMgrRef, eAssetType _type, const fs::path& _path);
    void ReloadAsset(AssetManager& _assetMgrRef, eAssetType _type, const fs::path& _path);
    void UnloadAsset(AssetManager& _assetMgrRef, eAssetType _type, const fs::path& _path);

    // 엔티티 관련 유틸리티
    void DestroyEntity(const Entity& _entity);
    void CloneEntity(const Entity& _entity);

    // 씬 관련 유틸리티
    void SaveScene(Scene* _pScene, const std::optional<fs::path>& _path = std::nullopt);
    void LoadScene(Scene* _pScene, const std::optional<fs::path>& _path = std::nullopt);

private:
    // message box
    void ShowModalBox_();
    void SetEditorThemeByJamEngine1_() const;
    void SetEditorThemeByMoonlight_() const;
    void SetEditorThemeByJamEngine2_() const;
    void SetEditorThemeByMaterial_() const;
    void SetEditorThemeByAdobe_() const;
    void SetEditorThemeByUnreal_() const;
    void SetEditorThemeByVisualStudio_() const;
    void ResetImGuiTheme_() const;

    bool m_panelShowBit[EnumCount<eEditorPanel>()];

    // panels
    ContentsBrowserPanel m_contentsBrowserPanel;
    EntityInspectorPanel m_entityInspectorPanel;
    AssetInspectorPanel  m_assetInspectorPanel;
    SceneHierarchyPanel  m_sceneHierarchyPanel;
    MainMenuBarPanel     m_mainMenuBarPanel;
    ViewportPanel        m_viewportPanel;
    ConsolePanel         m_consolePanel;
    DebugPanel           m_debugPanel;

    // shared data
    Entity m_editEntity = Entity::s_null;   // 현재 편집중인 엔티티

    // message box
    std::vector<Scope<IModalBoxPanel>> m_modalBoxPanelStack;   // 모달 박스 스택
    bool                               m_bShowModalBoxSignal = false;

    // 현재 테마
    eEditorTheme m_editorTheme = eEditorTheme::Default;   // 현재 에디터 테마

    constexpr static const char* const k_editorConfigFilename = "jamEditorConfig.json";   // 에디터 설정 파일 이름
};

}   // namespace jam
