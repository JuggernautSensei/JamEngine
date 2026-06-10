#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "AssetManager.h"
#include "Camera.h"
#include "CameraComponent.h"
#include "Events.h"
#include "Graphics.h"
#include "JsonArchive.h"
#include "Scene.h"
#include "SceneComponent.h"
#include "SceneDatabase.h"
#include "SceneNode.h"
#include "WindowsPlatform.h"

namespace Jam
{

SceneManager& SceneManager::GetInstance()
{
    return *s_pInstance;
}

void SceneManager::Resize(
    const Int32 _width,
    const Int32 _height) const
{
    if (m_pScene)
    {
        m_pScene->OnResize(_width, _height);
    }
}

void SceneManager::Update(
    const float _deltaSec) const
{
    // update active scene
    if (m_pScene)
    {
        // update scene
        m_pScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime() const
{
    Graphics&         gfx = GetGraphics();
    const BackBuffer& bb  = gfx.GetBackBuffer();
    const Viewport    vp  = { static_cast<UInt16>(bb.width), static_cast<UInt16>(bb.height) };
    RenderRuntime(kInvalidHandle, vp);
}

void SceneManager::RenderRuntime(
    const FrameBufferHandle _fbh,
    const Viewport          _vp) const
{
    if (m_pScene)
    {
        // update dirty transforms
        m_pScene->UpdateSceneGraph();

        // find first enabled camera
        RenderView renderView = { _fbh, _vp };
        for (const auto& [_, sc, cc]: m_pScene->Each<SceneComponent, CameraComponent>())
        {
            if (cc.bEnabled)
            {
                const Transform& transW     = sc.node.GetWorld();
                const Camera&    cmr        = cc.camera;
                renderView.pTransformOrNull = &transW;
                renderView.pCameraOrNull    = &cmr;
                break;
            }
        }

        // render
        m_pScene->OnRender(renderView);
    }
}

void SceneManager::Render(
    const RenderView& _view) const
{
    if (!m_pScene)
    {
        return;
    }

    m_pScene->UpdateSceneGraph();
    m_pScene->OnRender(_view);
}

void SceneManager::ImGuiRender() const
{
    if (!m_pScene)
    {
        return;
    }

    m_pScene->OnImGuiRender();
}

void SceneManager::LoadScene(
    Scoped<Scene>&& _pScene,
    const bool      _bDeferred)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    JAM_ASSERT(GetSceneDatabase().IsSceneRegistered(_pScene->GetTypeHash()), "Scene type is not reflected in the database");

    if (_bDeferred)
    {
        ChangeSceneDeferred_(std::move(_pScene));
    }
    else
    {
        ChangeScene_(std::move(_pScene));
    }
}

bool SceneManager::HasActiveScene() const
{
    return m_pScene != nullptr;
}

Scene* SceneManager::GetActiveScene() const
{
    JAM_ASSERT(m_pScene, "No active scene");
    return m_pScene.GetPtr();
}

Scene* SceneManager::GetActiveSceneOrNull() const
{
    return m_pScene ? m_pScene.GetPtr() : nullptr;
}

SceneManager::SceneManager()
{
    Application& app = GetApp();
    if (app.IsAppInitFlagsEnabled(eAppInitFlags::Subsystem_Editor))
    {
        m_bManageSceneResize = false;   // 에디터에서는 씬 메니저가 아닌, 에디터가 창 크기 변경 이벤트를 받아서 씬의 Resize 함수를 호출하도록 함
    }
}

SceneManager::~SceneManager()
{
    if (m_pScene)
    {
        m_pScene->OnExit();
    }
}

void SceneManager::CreateInstance()
{
    JAM_ASSERT(!s_pInstance, "SceneManager already initialized");
    s_pInstance = new SceneManager;
}

void SceneManager::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

void SceneManager::OnEvent(
    Event& _event)
{
    const Application& app = GetApp();
    if (!app.IsAppInitFlagsEnabled(eAppInitFlags::Subsystem_Editor))   // runtime mode
    {
        // handle window events in runtime mode
        EventDispatcher dispatcher { _event };
        dispatcher.Dispatch<WindowResizedEvent>(this, &SceneManager::OnWindowResizedEvent_);
        dispatcher.Dispatch<WindowMaximizedEvent>(this, &SceneManager::OnWindowMaximizeEvent_);
    }

    if (m_pScene)
    {
        m_pScene->OnEvent(_event);
    }
}

void SceneManager::OnWindowResizedEvent_(
    const WindowResizedEvent& _event) const
{
    if (m_bManageSceneResize)
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }
}

void SceneManager::OnWindowMaximizeEvent_(
    const WindowMaximizedEvent& _event) const
{
    if (m_bManageSceneResize)
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }
}

void SceneManager::ChangeSceneDeferred_(
    Scoped<Scene>&& _pScene)
{
    m_pNextScene     = std::move(_pScene);
    Application& app = GetApp();
    app.SubmitCommandToMainThread(Bind<&SceneManager::ChangeToNextScene_>(this));
}

void SceneManager::ChangeScene_(
    Scoped<Scene>&& _pScene)
{
    if (m_pScene)
    {
        m_pScene->OnExit();
    }

    m_pScene = std::move(_pScene);
    m_pScene->OnEnter();

    // resize in runtime.
    if (m_bManageSceneResize)
    {
        WindowsPlatform& win        = GetWindowsPlatform();
        const Vec2i      resolution = win.GetClientResolution();
        Resize(resolution.width, resolution.height);
    }

    // resource garbage collection
    Application& app = GetApp();
    if (app.IsAppInitFlagsEnabled(eAppInitFlags::Subsystem_AssetManager))
    {
        AssetManager& am = GetAssetManager();
        am.CleanGarbageAll();
    }

    // dispatch scene changed event
    SceneChangedEvent event = {};
    app.DispatchEvent(event);
}

void SceneManager::ChangeToNextScene_()
{
    JAM_ASSERT(m_pNextScene, "No next scene to change");
    ChangeScene_(std::move(m_pNextScene));
}

}   // namespace Jam