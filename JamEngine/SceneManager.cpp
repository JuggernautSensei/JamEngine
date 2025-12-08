#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "AssetManager.h"
#include "Camera.h"
#include "CameraComponent.h"
#include "Graphics.h"
#include "Scene.h"
#include "SceneDeserializer.h"
#include "Script.h"
#include "ScriptComponent.h"
#include "Transform.h"
#include "TransformHierarchy.h"

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

    void ChangeScene(const Ref<Scene>& _pScene)
    {
        struct ChangeSceneCommand
        {
            void operator()() const
            {
                Ref<Scene>& pScene = pCtx->pActiveScene;
                if (pScene)   // 기존 씬 퇴장
                {
                    pScene->OnExit();
                }

                pScene = pNextScene;                                 // change scene
                pScene->OnEnter();                                   // enter scene
                if (!Application::GetInstance().IsEditorEnabled())   // resize in runtime.
                {
                    auto [w, h] = Graphics::GetBackBufferResolution();
                    pCtx->Resize(w, h);
                }
                AssetManager::CleanGarbageAll();   // resource garbage collection

                // 씬 교체 이벤트 발생
                SceneChangedEvent event {};
                Application::GetInstance().DispatchEvent(event);
            }

            SceneManagerContext* pCtx       = nullptr;
            Ref<Scene>           pNextScene = nullptr;
        };

        // 다음 프레임에 씬 전환 작업 수행
        Application& app = Application::GetInstance();
        app.SubmitCommand(ChangeSceneCommand { this, _pScene });
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

    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;
    if (pScene)
    {
        pScene->OnExit();
    }

    JAM_DELETE(g_pSceneMgrCtx);
}

void SceneManager::OnEvent(Event& _event)
{
    Application& app = Application::GetInstance();
    if (!app.IsEditorEnabled())
    {
        // handle window events in runtime mode
        HandleEvent<WindowResizedEvent>(_event, g_pSceneMgrCtx, &SceneManagerContext::OnWindowResizedEvent);
        HandleEvent<WindowMaximizedEvent>(_event, g_pSceneMgrCtx, &SceneManagerContext::OnWindowMaximizeEvent);
    }

    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;
    if (pScene)
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
    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;

    // update active scene
    if (pScene)
    {
        // udpate script
        for (auto&& [_, scriptCmp]: pScene->Each<ScriptComponent>())
        {
            if (scriptCmp.pScript)
                scriptCmp.pScript->Update(_deltaSec);
        }

        // update scene
        pScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime()
{
    auto [w, h] = Graphics::GetBackBufferResolution();
    Viewport vp = Viewport { 0, 0, NarrowCast<UInt16>(w), NarrowCast<UInt16>(h) };
    RenderRuntime(k_invalidHandle, vp);
}

void SceneManager::RenderRuntime(FrameBufferHandle _handle, Viewport _viewport)
{
    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;
    if (pScene)
    {
        // update dirty transforms
        Entity root = pScene->GetRootEntity();
        root.GetTransformHierarchy().UpdateDirty_Recursive();

        // create view
        RenderView view { _handle, _viewport };

        // find first enabled camera
        for (const auto& [_, tc, cc]: pScene->Each<TransformComponent, CameraComponent>())
        {
            if (cc.enable)
            {
                const TransformHierarchy& th  = tc.transformHierarchy;
                const Camera&             cmr = cc.camera;

                view.eyePos = th.GetWorldTranslation();
                view.view   = th.GetWorldViewMatrix();
                view.proj   = cmr.CreateProjMatrix();
                break;
            }
        }

        // render
        pScene->OnRender(view);
    }
}

void SceneManager::Render(const RenderView& _view)
{
    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;
    if (pScene)
    {
        // update dirty transforms
        Entity root = pScene->GetRootEntity();
        root.GetTransformHierarchy().UpdateDirty_Recursive();

        // render
        pScene->OnRender(_view);
    }
}

void SceneManager::ImGuiRender()
{
    Ref<Scene>& pScene = g_pSceneMgrCtx->pActiveScene;

    if (pScene)
    {
        pScene->OnImGuiRender();
    }
}

bool SceneManager::LoadScene(const fs::path& _path)
{
    SceneDeserializer deserializer;
    if (!deserializer.LoadFromFile(_path))
    {
        return false;
    }

    if (!deserializer.Deserialize())
    {
        return false;
    }

    g_pSceneMgrCtx->ChangeScene(deserializer.GetScene());
    return true;
}

void SceneManager::LoadScene(const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    g_pSceneMgrCtx->ChangeScene(_pScene);
}

bool SceneManager::HasActiveScene()
{
    return !!g_pSceneMgrCtx->pActiveScene;
}

Ref<Scene>& SceneManager::GetActiveScene()
{
    JAM_ASSERT(g_pSceneMgrCtx->pActiveScene, "No active scene");
    return g_pSceneMgrCtx->pActiveScene;
}

}   // namespace jam