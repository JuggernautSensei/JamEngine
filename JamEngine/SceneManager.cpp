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

    void ChangeScene(const std::shared_ptr<Scene>& _pScene)
    {
        struct ChangeSceneCommnad
        {
            void operator()() const
            {
                std::shared_ptr<Scene>& pActiveScene = pCtx->pActiveScene;

                // 씬 퇴장
                if (pActiveScene)
                    pActiveScene->OnExit();

                pActiveScene = pNextScene;         // 씬 교체
                AssetManager::CleanGarbageAll();   // 씬 전환 후 사용하지 않는 에셋 정리

                // 씬 입장
                pActiveScene->OnEnter();
            }

            SceneMgrContext*       pCtx       = nullptr;
            std::shared_ptr<Scene> pNextScene = nullptr;
        };

        // 다음 프레임에 씬 전환 작업 수행
        Application& app = Application::GetInstance();
        app.SubmitCommand(ChangeSceneCommnad { this, _pScene });
    }

    std::shared_ptr<Scene> pActiveScene = nullptr;

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
    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;
    if (pActiveScene)
    {
        pActiveScene->OnExit();
        pActiveScene.reset();
    }

    JAM_DESTROY_TEXTURE(g_sceneManagerContext.sceneRenderTexture);
    JAM_DESTROY_FRAME_BUFFER(g_sceneManagerContext.sceneFrameBuffer);
}

void SceneManager::OnEvent(Event& _event)
{
    Application& app = Application::GetInstance();
    if (app.IsRuntime())
    {
        // editor mode인경우 Editor가 처리
        HandleEvent<WindowResizeEvent>(_event, &g_sceneManagerContext, &SceneMgrContext::OnWindowResizeEvent);
        HandleEvent<WindowMaximizeEvent>(_event, &g_sceneManagerContext, &SceneMgrContext::OnWindowMaximizeEvent);
    }

    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;
    if (pActiveScene)
        pActiveScene->OnEvent(_event);
}

void SceneManager::Resize(const Int32 _width, const Int32 _height)
{
    g_sceneManagerContext.Resize(_width, _height);
}

void SceneManager::Update(const float _deltaSec)
{
    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;

    // update active scene
    if (pActiveScene)
    {
        // udpate script
        for (auto&& [_, scriptCmp]: pActiveScene->Each<ScriptComponent>())
        {
            if (scriptCmp.pScript)
            {
                scriptCmp.pScript->Update(_deltaSec);
            }
        }

        pActiveScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime()
{
    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;

    if (pActiveScene)
    {
        // update transforms before rendering
        Entity root = pActiveScene->GetRootEntity();
        root.GetTransform().Update_Recursive();

        // scene view
        Int32 w = g_sceneManagerContext.sceneResolution.width;
        Int32 h = g_sceneManagerContext.sceneResolution.height;

        RenderView view = {
            g_sceneManagerContext.sceneFrameBuffer,
            Viewport { 0, 0, NarrowCast<UInt16>(w), NarrowCast<UInt16>(h) },
        };

        // find camera
        for (auto&& [_, cameraCmp, transformCmp]: pActiveScene->Each<CameraComponent, TransformComponent>())
        {
            if (cameraCmp.bMain)
            {
                const Transform& t = transformCmp.transform.GetWorld();
                const Camera&    c = cameraCmp.camera;

                view.eye  = t.GetTranslation();
                view.view = t.GetViewMatrix();
                view.proj = c.CreateProj();
                break;
            }
        }

        // render
        pActiveScene->OnRender(view);
        Graphics::BlitToBackBuffer(g_sceneManagerContext.sceneRenderTexture);
    }
}

void SceneManager::RenderOffScreen(const RenderView& _view)
{
    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;

    if (pActiveScene)
    {
        // update transforms before rendering
        Entity root = pActiveScene->GetRootEntity();
        root.GetTransform().Update_Recursive();

        // off-screen render
        pActiveScene->OnRender(_view);
    }
}

void SceneManager::ImGuiRender()
{
    std::shared_ptr<Scene>& pActiveScene = g_sceneManagerContext.pActiveScene;

    if (pActiveScene)
        pActiveScene->OnImGuiRender();
}

eErrorCode SceneManager::LoadScene(const fs::path& _scenePath)
{
    SceneSerializer serializer;

    // load
    eErrorCode ec0 = serializer.LoadFromFile(_scenePath);
    if (IsError(ec0))
        return ec0;

    // deserialize
    auto [ec1, pNewScene] = serializer.Deserialize();
    if (IsError(ec1))
        return ec1;

    g_sceneManagerContext.ChangeScene(pNewScene);
    return eErrorCode::Ok;
}

void SceneManager::LoadScene(const std::shared_ptr<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    g_sceneManagerContext.ChangeScene(_pScene);
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