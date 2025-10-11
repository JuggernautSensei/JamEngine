#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "Components.h"
#include "Events.h"
#include "Scene.h"
#include "SceneSerializer.h"
#include "Script.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

struct SceneMgrContext
{
    void OnWindowResizeEvent(const WindowResizeEvent& _event)
    {
        OnResize(_event.GetWidth(), _event.GetHeight());
    }

    void OnResize(const Int32 _width, const Int32 _height)
    {
        JAM_DESTROY_TEXTURE(sceneTexture);
        JAM_DESTROY_FRAME_BUFFER(sceneFrameBuffer);

        sceneTexture     = gfx::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
        sceneFrameBuffer = gfx::CreateFrameBuffer(sceneTexture);
    }

    std::unique_ptr<Scene> pActiveScene = nullptr;
    std::unique_ptr<Scene> pNextScene   = nullptr;

    FrameBufferHandle sceneFrameBuffer = k_invalidHandle;
    TextureHandle     sceneTexture     = k_invalidHandle;
};
static SceneMgrContext g_sceneMgrCtx;

////////////////////////////////////////////////////////////

namespace jam::scene_manager
{

void Init()
{
    Application&  app    = Application::GetInstance();
    const Window& win    = app.GetWindow();
    auto [width, height] = win.GetWindowSize();
    g_sceneMgrCtx.OnResize(width, height);
}

void Shutdown()
{
    JAM_DESTROY_TEXTURE(g_sceneMgrCtx.sceneTexture);
    JAM_DESTROY_FRAME_BUFFER(g_sceneMgrCtx.sceneFrameBuffer);
}

void OnEvent(Event& _event)
{
    HandleEvent<WindowResizeEvent>(_event, &g_sceneMgrCtx, &SceneMgrContext::OnWindowResizeEvent);
    if (g_sceneMgrCtx.pActiveScene)
    {
        g_sceneMgrCtx.pActiveScene->OnEvent(_event);
    }
}

void Update(const float _deltaSec)
{
    // change scene
    if (g_sceneMgrCtx.pNextScene)
    {
        if (g_sceneMgrCtx.pActiveScene)
        {
            g_sceneMgrCtx.pActiveScene->OnExit();
        }

        g_sceneMgrCtx.pActiveScene = std::move(g_sceneMgrCtx.pNextScene);
        g_sceneMgrCtx.pActiveScene->OnEnter();
    }

    // update active scene
    if (g_sceneMgrCtx.pActiveScene)
    {
        // udpate script
        for (auto&& [_, scriptCmp]: g_sceneMgrCtx.pActiveScene->CreateView<ScriptComponent>().each())
        {
            if (scriptCmp.pScript)
            {
                scriptCmp.pScript->Update(_deltaSec);
            }
        }

        g_sceneMgrCtx.pActiveScene->OnUpdate(_deltaSec);
    }
}

void RenderRuntime()
{
    if (g_sceneMgrCtx.pActiveScene)
    {
        Scene* pScene = g_sceneMgrCtx.pActiveScene.get();

        // 렌더링 하기 전에 Transform 갱신
        Entity              root      = pScene->GetRootEntity();
        TransformComponent& rootTrans = root.GetTransform();
        rootTrans.transform.Update_Recursive();

        // find camera
        const Camera*    pMainCamera    = nullptr;
        const Transform* pMainTransform = nullptr;
        for (auto&& [_, cameraCmp, transformCmp]: pScene->CreateView<CameraComponent, TransformComponent>().each())
        {
            if (cameraCmp.bMain)
            {
                pMainCamera    = &cameraCmp.camera;
                pMainTransform = &transformCmp.transform.GetWorld();
                break;
            }
        }

        if (pMainCamera && pMainTransform)
        {
            pScene->OnRender(g_sceneMgrCtx.sceneFrameBuffer, *pMainCamera, *pMainTransform);
            gfx::BlitToBackBuffer(g_sceneMgrCtx.sceneTexture);
        }
    }
}

void Render(const FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform)
{
    if (g_sceneMgrCtx.pActiveScene)
    {
        Scene* pScene = g_sceneMgrCtx.pActiveScene.get();

        // 렌더링 하기 전에 Transform 갱신
        Entity              root      = pScene->GetRootEntity();
        TransformComponent& rootTrans = root.GetTransform();
        rootTrans.transform.Update_Recursive();

        // render
        g_sceneMgrCtx.pActiveScene->OnRender(_handle, _camera, _transform);
    }
}

eErrorCode LoadScene(const fs::path& _scenePath)
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

        g_sceneMgrCtx.pNextScene = std::move(pScene);
    }
    return eErrorCode::Ok;
}

void LoadScene(std::unique_ptr<Scene>&& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    g_sceneMgrCtx.pNextScene = std::move(_pScene);
}

Scene* GetActiveScene()
{
    JAM_ASSERT(g_sceneMgrCtx.pActiveScene, "No active scene");
    return g_sceneMgrCtx.pActiveScene.get();
}

}   // namespace jam::scene_manager