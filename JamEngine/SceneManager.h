#pragma once
#include "RenderView.h"
#include "Scene.h"

namespace Jam
{
class WindowMaximizedEvent;
class WindowResizedEvent;

class Event;

class SceneManager
{
public:
    SceneManager(const SceneManager&)            = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&)                 = delete;
    SceneManager& operator=(SceneManager&&)      = delete;

    [[nodiscard]] static SceneManager& GetInstance();

    void Resize(Int32 _width, Int32 _height) const;
    void Update(float _deltaSec) const;

    void RenderRuntime() const;                                       // 백버퍼에 메인 카메라로 렌더링
    void RenderRuntime(FrameBufferHandle _fbh, Viewport _vp) const;   // 특정 버퍼에 메인 카메라로 렌더링
    void Render(const RenderView& _view) const;

    void ImGuiRender() const;

    [[nodiscard]] Error LoadScene(const Path& _path);
    void                LoadScene(const Ref<Scene>& _pScene);

    [[nodiscard]] bool              HasActiveScene() const;
    [[nodiscard]] const Ref<Scene>& GetActiveScene() const;

private:
    SceneManager() = default;
    ~SceneManager();

    static void CreateInstance();
    static void DestroyInstance();

    void OnEvent(Event& _event);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event) const;
    void OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event) const;

    void ChangeSceneDefered_(const Ref<Scene>& _pScene);
    void ChangeScene_(const Ref<Scene>& _pScene);

    Ref<Scene> m_pActiveScene = nullptr;

    inline static SceneManager* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_SCENE_MGR Jam::SceneManager::GetInstance()