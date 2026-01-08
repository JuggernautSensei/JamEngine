#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "AssetManager.h"
#include "Camera.h"
#include "CameraComponent.h"
#include "Graphics.h"
#include "Scene.h"
#include "SceneComponent.h"
#include "SceneDeserializer.h"
#include "SceneNode.h"
#include "Script.h"
#include "ScriptComponent.h"

namespace jam
{

struct SceneManagerContext
{
    void OnWindowResizedEvent(const WindowResizedEvent& _event) const
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }

    void OnWindowMaximizeEvent(const WindowMaximizedEvent& _event) const
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }

    void Resize(const Int32 _width, const Int32 _height) const
    {
        if (pActiveScene)
        {
            pActiveScene->OnResize(_width, _height);
        }
    }

    Ref<Scene> pActiveScene = nullptr;

}* g_pSceneMgrCtx = nullptr;

}   // namespace jam

namespace jam
{

void SceneManager::Init()
{
    JAM_ASSERT(!g_pSceneMgrCtx, "SceneManager already initialized");
    g_pSceneMgrCtx = JAM_NEW(SceneManagerContext);
}

void SceneManager::Shutdown()
{
    JAM_ASSERT(g_pSceneMgrCtx, "SceneManager not initialized");
    if (const Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        pScene->OnExit();
    }

    JAM_DELETE(g_pSceneMgrCtx);
}

void SceneManager::OnEvent(Event& _event)
{
    const Application& app = Application::GetInstance();
    if (!app.IsEditorEnabled())   // runtime mode
    {
        // handle window events in runtime mode
        HandleEvent<WindowResizedEvent>(_event, g_pSceneMgrCtx, &SceneManagerContext::OnWindowResizedEvent);
        HandleEvent<WindowMaximizedEvent>(_event, g_pSceneMgrCtx, &SceneManagerContext::OnWindowMaximizeEvent);
    }

    if (const Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        pScene->OnEvent(_event);
    }
}

void SceneManager::Resize(const Int32 _width, const Int32 _height)
{
    g_pSceneMgrCtx->Resize(_width, _height);
}

void SceneManager::Update(const float _deltaSec)
{
    // update active scene
    if (const Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        // udpate script
        for (const auto&& [_, sc]: pScene->Each<ScriptComponent>())
        {
            if (sc.pScript)
            {
                sc.pScript->Update(_deltaSec);
            }
        }

        // update scene
        pScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime()
{
    const auto [w, h] = Graphics::GetBackBufferResolution();
    const Viewport vp = { NarrowCast<UInt16>(w), NarrowCast<UInt16>(h) };
    RenderRuntime(k_invalidHandle, vp);
}

void SceneManager::RenderRuntime(
    FrameBufferHandle _fbh,
    Viewport          _vp)
{
    if (Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        // update dirty transforms
        pScene->UpdateSceneGraph_();

        // create view
        RenderView view = { _fbh, _vp };

        // find first enabled camera
        for (const auto&& [_, sc, cc]: pScene->Each<SceneComponent, CameraComponent>())
        {
            if (cc.bEnabled)
            {
                const Transform& wt  = sc.node.GetWorldTransform();
                const Camera&    cmr = cc.camera;

                view.eye  = wt.GetTranslation();
                view.view = wt.ToViewMatrix();
                view.proj = cmr.ToProjMatrix();
                break;
            }
        }

        // render
        pScene->OnRender(view);
    }
}

void SceneManager::Render(const RenderView& _view)
{
    if (const Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        pScene->UpdateSceneGraph_();
        pScene->OnRender(_view);
    }
}

void SceneManager::ImGuiRender()
{
    if (const Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene)
    {
        pScene->OnImGuiRender();
    }
}

bool SceneManager::LoadScene(const fs::path& _path)
{
    SceneDeserializer deserializer;
    if (!deserializer.LoadJsonFile(_path))
    {
        return false;
    }

    if (!deserializer.Deserialize())
    {
        return false;
    }

    ChangeSceneDefered_(deserializer.GetScene());
    return true;
}

void SceneManager::LoadScene(const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    ChangeSceneDefered_(_pScene);
}

bool SceneManager::HasActiveScene()
{
    return g_pSceneMgrCtx->pActiveScene != nullptr;
}

Ref<Scene>& SceneManager::GetActiveScene()
{
    JAM_ASSERT(g_pSceneMgrCtx->pActiveScene, "No active scene");
    return g_pSceneMgrCtx->pActiveScene;
}

void SceneManager::ChangeSceneDefered_(
    const Ref<Scene>& _pScene)
{
    Application& app = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_FUNCTION(ChangeScene_, _pScene));
}

void SceneManager::ChangeScene_(
    const Ref<Scene>& _pScene)
{
    Ref<Scene>& pActiveScene = g_pSceneMgrCtx->pActiveScene;
    if (pActiveScene)
    {
        pActiveScene->OnExit();
    }

    pActiveScene = _pScene;
    pActiveScene->OnEnter();

    // resize in runtime.
    if (!Application::GetInstance().IsEditorEnabled())
    {
        const auto [w, h] = Graphics::GetBackBufferResolution();
        g_pSceneMgrCtx->Resize(w, h);
    }

    // resource garbage collection
    AssetManager::CleanGarbageAll();

    // dispatch scene changed event
    SceneChangedEvent event = {};
    Application::GetInstance().DispatchEvent(event);
}

}   // namespace jam