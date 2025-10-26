#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "AssetManager.h"
#include "Components.h"
#include "Graphics.h"
#include "Scene.h"
#include "SceneSerializer.h"
#include "Script.h"

namespace jam
{

struct SceneMgrContext
{
    void OnWindowResizeEvent(const WindowResizeEvent& _event)
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }

    void OnWindowMaximizeEvent(const WindowMaximizeEvent& _event)
    {
        Resize(_event.GetWidth(), _event.GetHeight());
    }

    void Resize(const Int32 _width, const Int32 _height)
    {
        JAM_DESTROY_TEXTURE(sceneRenderTexture);
        JAM_DESTROY_FRAME_BUFFER(sceneFrameBuffer);

        sceneRenderTexture = Graphics::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget | eTextureCreateFlags_WriteOnly);
        sceneFrameBuffer   = Graphics::CreateFrameBuffer(sceneRenderTexture);
        sceneResolution    = { _width, _height };

        if (pActiveScene)
            pActiveScene->OnResize(_width, _height);
    }

    std::unique_ptr<Scene> pActiveScene = nullptr;
    std::unique_ptr<Scene> pNextScene   = nullptr;

    // 씬 프레임 버퍼
    TextureHandle     sceneRenderTexture = k_invalidHandle;
    FrameBufferHandle sceneFrameBuffer   = k_invalidHandle;
    Resolution        sceneResolution    = {};
} g_sceneManagerContext;

}   // namespace jam

namespace jam
{

void SceneManager::Init()
{
    // 초기 해상도는 클라이언트 영역 크기로 설정
    Application&  app = Application::GetInstance();
    const Window& win = app.GetWindow();
    auto [w, h]       = win.GetClientResolution();
    g_sceneManagerContext.Resize(w, h);
}

void SceneManager::Shutdown()
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;
    if (pCurScene)
    {
        pCurScene->OnExit();
        pCurScene.reset();
    }

    JAM_DESTROY_TEXTURE(g_sceneManagerContext.sceneRenderTexture);
    JAM_DESTROY_FRAME_BUFFER(g_sceneManagerContext.sceneFrameBuffer);
}

void SceneManager::OnEvent(Event& _event)
{
    Application& app = Application::GetInstance();
    if (app.IsRuntime())
    {
        HandleEvent<WindowResizeEvent>(_event, &g_sceneManagerContext, &SceneMgrContext::OnWindowResizeEvent);
        HandleEvent<WindowMaximizeEvent>(_event, &g_sceneManagerContext, &SceneMgrContext::OnWindowMaximizeEvent);
    }

    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;
    if (pCurScene)
        pCurScene->OnEvent(_event);
}

void SceneManager::Resize(const Int32 _width, const Int32 _height)
{
    g_sceneManagerContext.Resize(_width, _height);
}

void SceneManager::Update(const float _deltaSec)
{
    std::unique_ptr<Scene>& pCurScene  = g_sceneManagerContext.pActiveScene;
    std::unique_ptr<Scene>& pNextScene = g_sceneManagerContext.pNextScene;

    // change scene
    if (pNextScene)
    {
        if (pCurScene)
            pCurScene->OnExit();

        pCurScene = std::move(pNextScene);
        AssetManager::CleanGarbageAll();   // 씬 전환 후 사용하지 않는 에셋 정리

        // 씬 입장
        pCurScene->OnEnter();
    }

    // update active scene
    if (pCurScene)
    {
        // udpate script
        for (auto&& [_, scriptCmp]: pCurScene->Each<ScriptComponent>())
        {
            if (scriptCmp.pScript)
            {
                scriptCmp.pScript->Update(_deltaSec);
            }
        }

        pCurScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime()
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;

    if (pCurScene)
    {
        // update transforms before rendering
        Entity root = pCurScene->GetRootEntity();
        root.GetTransform().Update_Recursive();

        // scene view
        Int32 w = g_sceneManagerContext.sceneResolution.width;
        Int32 h = g_sceneManagerContext.sceneResolution.height;

        SceneView view = {
            g_sceneManagerContext.sceneFrameBuffer,
            Viewport { 0, 0, NarrowCast<UInt16>(w), NarrowCast<UInt16>(h) },
        };

        // find camera
        for (auto&& [_, cameraCmp, transformCmp]: pCurScene->Each<CameraComponent, TransformComponent>())
        {
            if (cameraCmp.bMain)
            {
                const Transform& t = transformCmp.transform.GetWorld();
                view.eye           = t.translate;
                view.view          = t.CreateView();
                view.proj          = cameraCmp.CreateProj();
                break;
            }
        }

        // render
        pCurScene->OnRender(view);
        Graphics::BlitToBackBuffer(g_sceneManagerContext.sceneRenderTexture);
    }
}

void SceneManager::RenderOffScreen(const SceneView& _view)
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;

    if (pCurScene)
    {
        // update transforms before rendering
        Entity root = pCurScene->GetRootEntity();
        root.GetTransform().Update_Recursive();

        // off-screen render
        pCurScene->OnRender(_view);
    }
}

void SceneManager::ImGuiRender()
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;

    if (pCurScene)
    {
        pCurScene->OnImGuiRender();
    }
}

eErrorCode SceneManager::LoadScene(const fs::path& _scenePath)
{
    SceneSerializer serializer;

    // load
    {
        eErrorCode ec = serializer.LoadFromFile(_scenePath);
        if (IsError(ec))
        {
            return ec;
        }
    }

    // deserialize
    {
        auto [ec, pScene] = serializer.Deserialize();
        if (IsError(ec))
        {
            return ec;
        }

        g_sceneManagerContext.pNextScene = std::move(pScene);
    }
    return eErrorCode::Ok;
}

void SceneManager::LoadScene(std::unique_ptr<Scene>&& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    g_sceneManagerContext.pNextScene = std::move(_pScene);
}

Scene* SceneManager::GetActiveScene()
{
    JAM_ASSERT(g_sceneManagerContext.pActiveScene, "No active scene");
    return g_sceneManagerContext.pActiveScene.get();
}

Resolution SceneManager::GetSceneResoultion()
{
    return g_sceneManagerContext.sceneResolution;
}

}   // namespace jam