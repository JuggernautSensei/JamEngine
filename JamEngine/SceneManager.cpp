#include "pch.h"

#include "SceneManager.h"

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
    void Resize(const Int32 _width, const Int32 _height)
    {
        JAM_DESTROY_TEXTURE(sceneTexture);
        JAM_DESTROY_FRAME_BUFFER(sceneFrameBuffer);

        sceneTexture     = Graphics::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
        sceneFrameBuffer = Graphics::CreateFrameBuffer(sceneTexture);
    }

    std::unique_ptr<Scene> pActiveScene = nullptr;
    std::unique_ptr<Scene> pNextScene   = nullptr;

    // 씬 프레임 버퍼
    FrameBufferHandle sceneFrameBuffer = k_invalidHandle;
    TextureHandle     sceneTexture     = k_invalidHandle;
} g_sceneManagerContext;

}   // namespace jam

namespace jam
{

void SceneManager::Init()
{
    g_sceneManagerContext.Resize(800, 600);   // 기본 크기
}

void SceneManager::Shutdown()
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;
    if (pCurScene)
    {
        pCurScene->OnExit();
        pCurScene.reset();
    }

    JAM_DESTROY_TEXTURE(g_sceneManagerContext.sceneTexture);
    JAM_DESTROY_FRAME_BUFFER(g_sceneManagerContext.sceneFrameBuffer);
}

void SceneManager::OnEvent(Event& _event)
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;
    if (pCurScene)
    {
        pCurScene->OnEvent(_event);
    }
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
        {
            pCurScene->OnExit();
        }
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
        // 렌더링 하기 전에 Transform 갱신
        Entity              rootE = pCurScene->GetRootEntity();
        TransformHierarchy& rootT = rootE.GetTransform();
        rootT.Update_Recursive();

        // find camera
        const Camera*    pMainCamera    = nullptr;
        const Transform* pMainTransform = nullptr;
        for (auto&& [_, cameraCmp, transformCmp]: pCurScene->Each<CameraComponent, TransformComponent>())
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
            pCurScene->OnRender(g_sceneManagerContext.sceneFrameBuffer, *pMainCamera, *pMainTransform);
            Graphics::BlitToBackBuffer(g_sceneManagerContext.sceneTexture);
        }
    }
}

void SceneManager::RenderOffScreen(const FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform)
{
    std::unique_ptr<Scene>& pCurScene = g_sceneManagerContext.pActiveScene;

    if (pCurScene)
    {
        // 렌더링 하기 전에 Transform 갱신
        Entity              rootE = pCurScene->GetRootEntity();
        TransformHierarchy& rootT = rootE.GetTransform();
        rootT.Update_Recursive();

        pCurScene->OnRender(_handle, _camera, _transform);
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

}   // namespace jam