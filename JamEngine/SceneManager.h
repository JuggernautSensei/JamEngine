#pragma once

namespace Jam
{
class WindowMaximizedEvent;
class WindowResizedEvent;

class Event;
class Scene;
struct RenderView;

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

    void LoadScene(Scoped<Scene>&& _pScene, bool _bDeferred = true);

    [[nodiscard]] bool   HasActiveScene() const;
    [[nodiscard]] Scene* GetActiveScene() const;
    [[nodiscard]] Scene* GetActiveSceneOrNull() const;

private:
    SceneManager();
    ~SceneManager();

    static void CreateInstance();
    static void DestroyInstance();

    void OnEvent(Event& _event);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event) const;
    void OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event) const;

    void ChangeSceneDeferred_(Scoped<Scene>&& _pScene);
    void ChangeScene_(Scoped<Scene>&& _pScene);
    void ChangeToNextScene_();

    Scoped<Scene> m_pScene             = nullptr;
    Scoped<Scene> m_pNextScene         = nullptr;
    bool          m_bManageSceneResize = true;

    inline static SceneManager* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline SceneManager& GetSceneManager()
{
    return SceneManager::GetInstance();
}

}   // namespace Jam