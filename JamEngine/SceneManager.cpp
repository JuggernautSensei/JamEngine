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
    Graphics&            gfx   = GetGraphics();
    const GraphicsStats& stats = gfx.GetStats();
    const Viewport       vp    = { NarrowCast<UInt16>(stats.backBufferWidth), NarrowCast<UInt16>(stats.backBufferHeight) };
    RenderRuntime(k_invalidHandle, vp);
}

void SceneManager::RenderRuntime(
    const FrameBufferHandle _fbh,
    const Viewport          _vp) const
{
    if (m_pScene)
    {
        // update dirty transforms
        m_pScene->UpdateSceneGraph();

        // create view
        RenderView view = { _fbh, _vp };

        // find first enabled camera
        for (const auto& [_, sc, cc]: m_pScene->Each<SceneComponent, CameraComponent>())
        {
            if (cc.bEnabled)
            {
                const Transform& transW = sc.node.GetWorldTransform();
                const Camera&    cmr    = cc.camera;

                view.eyePos = transW.GetTranslation();
                view.view   = transW.GetMatrix().Invert();
                view.proj   = cmr.GetProjMatrix();
                break;
            }
        }

        // render
        m_pScene->OnRender(view);
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
    const bool      _bDefered)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    JAM_ASSERT(GetSceneDatabase().IsSceneReflected(_pScene->GetTypeHash()), "Scene type is not reflected in the database");

    if (_bDefered)
    {
        ChangeSceneDefered_(std::move(_pScene));
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
    if (!GetApp().IsCoreFlagsEnabled(eAppCoreFlags::Editor))   // runtime mode
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
    Resize(_event.GetWidth(), _event.GetHeight());
}

void SceneManager::OnWindowMaximizeEvent_(
    const WindowMaximizedEvent& _event) const
{
    Resize(_event.GetWidth(), _event.GetHeight());
}

void SceneManager::ChangeSceneDefered_(
    Scoped<Scene>&& _pScene)
{
    m_pNextScene = std::move(_pScene);
    GetApp().SubmitCommandToMainThread(Bind<&SceneManager::ChangeToNextScene_>(this));
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
    Application& app = GetApp();
    if (!app.IsCoreFlagsEnabled(eAppCoreFlags::Editor))   // runtime mode
    {
        Graphics&            gfx   = GetGraphics();
        const GraphicsStats& stats = gfx.GetStats();
        Resize(stats.backBufferWidth, stats.backBufferHeight);
    }

    // resource garbage collection
    if (app.IsCoreFlagsEnabled(eAppCoreFlags::AssetManager))
    {
        GetAssetManager().CleanGarbageAll();
    }

    // dispatch scene changed event
    SceneChangedEvent event = {};
    GetApp().DispatchEvent(event);
}

void SceneManager::ChangeToNextScene_()
{
    JAM_ASSERT(m_pNextScene, "No next scene to change");
    ChangeScene_(std::move(m_pNextScene));
}

}   // namespace Jam