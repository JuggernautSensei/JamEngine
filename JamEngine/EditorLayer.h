#pragma once
#include "Entity.h"
#include "Event.h"
#include "ILayer.h"
#include "Textures.h"

namespace jam
{
class BackBufferCleanupEvent;
}

namespace jam
{
class WindowResizeEvent;
}

namespace jam
{

enum eEditorPanelShowFlags_ : Int32
{
    eEditorPanelShowFlags_None,
    eEditorPanelShowFlags_DebugPanel      = 1 << 0,
    eEditorPanelShowFlags_Viewport        = 1 << 1,
    eEditorPanelShowFlags_SceneHierarchy  = 1 << 2,
    eEditorPanelShowFlags_AssetInspector  = 1 << 3,
    eEditorPanelShowFlags_EntityInspector = 1 << 4,
    eEditorPanelShowFlags_ContentsBrowser = 1 << 5,
    eEditorPanelShowFlags_Console         = 1 << 6,
};
using eEditorPanelShowFlags = Int32;

class EditorLayer : public ILayer
{
    // for content browser
    struct Content
    {
        fs::path                  path;
        std::string               name;
        std::optional<eAssetType> assetTypeOrNull;   // if content available as asset, this will be set. otherwise, std::nullopt

        Texture2D thumbnail;
    };

    // for content browser
    struct Directory
    {
        fs::path               path;
        std::string            name;
        std::vector<Directory> subDirectories;
    };

public:
    EditorLayer();
    ~EditorLayer() override;

    EditorLayer(const EditorLayer&)                = default;
    EditorLayer& operator=(const EditorLayer&)     = default;
    EditorLayer(EditorLayer&&) noexcept            = default;
    EditorLayer& operator=(EditorLayer&&) noexcept = default;

    void OnUpdate(float _deltaTime) override;
    void OnRender() override;
    void OnEvent(Event& _event) override;

    void SetShowFlags(eEditorPanelShowFlags _flags, bool _bEnable);
    void ToggleShowFlags(eEditorPanelShowFlags _flags);

    // use this to show a message box -> may be very useful!
    void OpenMessageBox(std::string_view _title, const std::function<void()>& _drawCallback);

private:
    void ShowMainMenuBar_();
    void ShowDebugPanel_();
    void ShowViewport_();
    void ShowSceneHierarchy_();
    void ShowEntityInspector_();
    void ShowAssetInspector_();
    void ShowContentsBrowser_();
    void ShowConsole_();

    // event handlers
    void CreateScreenDependentResources_(Int32 _width, Int32 _height);
    void OnWindowResizeEvent_(const WindowResizeEvent& e);

    // message box
    void ShowMessageBox_();

    // content browser
    void UpdateFocusDirectoryContents();
    void UpdateRootDirectory();
    void SetFocusDirectory(const fs::path& _path);
    void SetFocusDirectoryToParentDirectory();

    // draw widget
    void DrawWidgetButton(const Content& _content, const ImVec2& _buttonSize);

    eEditorPanelShowFlags m_flags          = eEditorPanelShowFlags_None;
    Entity                m_selectedEntity = Entity::s_null;

    // message box
    std::function<void()> m_messageBoxDrawCallback;
    std::string           m_messageBoxTitle;
    bool                  m_bShowMessageBox = false;

    // viewport
    Texture2D m_viewportTexture;

    // layer interface
    EventDispatcher m_dispatcher;

    // content browser
    fs::path             m_focusDirectoryPath;       // 현재 디렉토리의 경로
    std::vector<Content> m_focusDirectoryContents;   // 현재 디렉토리의 컨텐츠
    Directory            m_rootDirectory;            // root directory = contents directory. 전체적인 폴더 구조만 저장
    Texture2D            m_defaultFileIcon;
};

}   // namespace jam
