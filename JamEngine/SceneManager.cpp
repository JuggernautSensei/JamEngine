#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "AssetManager.h"
#include "Components.h"
#include "CullingSystem.h"
#include "Events.h"
#include "Gfx.h"
#include "Profile.h"
#include "Scene.h"
#include "SceneSerializer.h"
#include "Script.h"

namespace jam::scene_manager
{

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

struct SceneMgrContext
{
    void OnWindowResizeEvent(const WindowResizeEvent& _event)
    {
        OnResize(_event.GetWidth(), _event.GetHeight());
    }

    void OnWindowMaximizeEvent(const WindowMaximizeEvent& _event)
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

    // 씬 프레임 버퍼
    FrameBufferHandle sceneFrameBuffer = k_invalidHandle;
    TextureHandle     sceneTexture     = k_invalidHandle;
} g_ctx;

////////////////////////////////////////////////////////////

void Init(const Int32 _width, const Int32 _height)
{
    g_ctx.OnResize(_width, _height);
}

void Shutdown()
{
    if (g_ctx.pActiveScene)
    {
        g_ctx.pActiveScene->OnExit();
        g_ctx.pActiveScene.reset();
    }

    JAM_DESTROY_TEXTURE(g_ctx.sceneTexture);
    JAM_DESTROY_FRAME_BUFFER(g_ctx.sceneFrameBuffer);
}

void OnEvent(Event& _event)
{
    HandleEvent<WindowResizeEvent>(_event, &g_ctx, &SceneMgrContext::OnWindowResizeEvent);
    HandleEvent<WindowMaximizeEvent>(_event, &g_ctx, &SceneMgrContext::OnWindowMaximizeEvent);
    if (g_ctx.pActiveScene)
    {
        g_ctx.pActiveScene->OnEvent(_event);
    }
}

void Update(const float _deltaSec)
{
    // change scene
    if (g_ctx.pNextScene)
    {
        if (g_ctx.pActiveScene)
        {
            g_ctx.pActiveScene->OnExit();
        }
        g_ctx.pActiveScene = std::move(g_ctx.pNextScene);
        asset_manager::CleanGarbageAll();   // 씬 전환 후 사용하지 않는 에셋 정리

        // 씬 입장
        g_ctx.pActiveScene->OnEnter();
    }

    // update active scene
    if (g_ctx.pActiveScene)
    {
        // udpate script
        for (auto&& [_, scriptCmp]: g_ctx.pActiveScene->Each<ScriptComponent>())
        {
            if (scriptCmp.pScript)
            {
                scriptCmp.pScript->Update(_deltaSec);
            }
        }

        g_ctx.pActiveScene->OnUpdate(_deltaSec);
    }
}

void RenderRuntime()
{
    if (g_ctx.pActiveScene)
    {
        std::unique_ptr<Scene>& pScene = g_ctx.pActiveScene;

        // 렌더링 하기 전에 Transform 갱신
        Entity              rootE = pScene->GetRootEntity();
        HierarchyTransform& rootT = rootE.GetHierarchyTransform();
        rootT.Update_Recursive();

        // find camera
        const Camera*    pMainCamera    = nullptr;
        const Transform* pMainTransform = nullptr;
        for (auto&& [_, cameraCmp, transformCmp]: pScene->View<CameraComponent, TransformComponent>().each())
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
            pScene->OnRender(g_ctx.sceneFrameBuffer, *pMainCamera, *pMainTransform);
            gfx::BlitToBackBuffer(g_ctx.sceneTexture);
        }
    }
}

void RenderOffScreen(const FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform)
{
    if (g_ctx.pActiveScene)
    {
        std::unique_ptr<Scene>& pScene = g_ctx.pActiveScene;

        // 렌더링 하기 전에 Transform 갱신
        Entity              rootE = pScene->GetRootEntity();
        HierarchyTransform& rootT = rootE.GetHierarchyTransform();
        rootT.Update_Recursive();

        pScene->OnRender(_handle, _camera, _transform);
    }
}

void ImGuiRender()
{
    if (g_ctx.pActiveScene)
    {
        g_ctx.pActiveScene->OnImGuiRender();
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

        g_ctx.pNextScene = std::move(pScene);
    }
    return eErrorCode::Ok;
}

void LoadScene(std::unique_ptr<Scene>&& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    g_ctx.pNextScene = std::move(_pScene);
}

Scene* GetActiveScene()
{
    JAM_ASSERT(g_ctx.pActiveScene, "No active scene");
    return g_ctx.pActiveScene.get();
}

}   // namespace jam::scene_manager