#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "Components.h"
#include "Events.h"
#include "Scene.h"
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

    HashMap<std::string, std::unique_ptr<Scene>> sceneMap = {};

    Scene* pActiveScene = nullptr;
    Scene* pNextScene   = nullptr;

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
}

void Update(const float _deltaSec)
{
    // change scene
    if (g_sceneMgrCtx.pNextScene)
    {
        // 기존 씬 탈출
        if (g_sceneMgrCtx.pActiveScene)
        {
            g_sceneMgrCtx.pActiveScene->OnExit();
        }

        // 씬 변경
        g_sceneMgrCtx.pActiveScene = g_sceneMgrCtx.pNextScene;
        g_sceneMgrCtx.pNextScene   = nullptr;

        // 씬 진입
        g_sceneMgrCtx.pActiveScene->OnEnter();

        // 씬 변경 완료
        std::string      activeSceneName { g_sceneMgrCtx.pActiveScene->GetName() };
        SceneChangeEvent event { activeSceneName };
        Application&     app = Application::GetInstance();
        app.DispatchEvent(event);
    }

    // update active scene
    if (g_sceneMgrCtx.pActiveScene)
    {
        // update scripts
        for (auto&& [_, scriptCmp]: g_sceneMgrCtx.pActiveScene->CreateView<ScriptComponent>().each())
        {
            if (scriptCmp.pScript)
            {
                scriptCmp.pScript->Update(_deltaSec);
            }
        }

        // update scene
        g_sceneMgrCtx.pActiveScene->OnUpdate(_deltaSec);
    }
}

void RenderRuntime()
{
    // find camera
    const Camera*    pMainCamera    = nullptr;
    const Transform* pMainTransform = nullptr;

    for (auto&& [_, cameraCmp, transformCmp]: g_sceneMgrCtx.pActiveScene->CreateView<CameraComponent, TransformComponent>().each())
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
        g_sceneMgrCtx.pActiveScene->OnRender(g_sceneMgrCtx.sceneFrameBuffer, *pMainCamera, *pMainTransform);
        gfx::BlitToBackBuffer(g_sceneMgrCtx.sceneTexture);   // 화면에 출력
    }
}

void Render(const FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform)
{
    if (g_sceneMgrCtx.pActiveScene)
    {
        g_sceneMgrCtx.pActiveScene->OnRender(_handle, _camera, _transform);
    }
}

Scene* AddScene(std::string_view _name, std::unique_ptr<Scene>&& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene cannot be added.");
    JAM_ASSERT(!HasScene(_name), "Scene with name '{}' already exists.", _name);

    auto [iter, bResult] = g_sceneMgrCtx.sceneMap.emplace(std::string(_name), std::move(_pScene));
    JAM_ASSERT(bResult, "Failed to add scene with name '{}'.", _name);
    return iter->second.get();
}

void RemoveScene(std::string_view _name)
{
    auto iter = g_sceneMgrCtx.sceneMap.find(std::string(_name));
    JAM_ASSERT(iter != g_sceneMgrCtx.sceneMap.end(), "Scene with name '{}' does not exist.", _name);
    JAM_ASSERT(iter->second.get() != g_sceneMgrCtx.pActiveScene, "Cannot remove the active scene.");
    g_sceneMgrCtx.sceneMap.erase(iter);
}

void ChangeScene(const std::string_view _name)
{
    g_sceneMgrCtx.pNextScene = GetScene(_name);
}

bool HasActiveScene()
{
    return g_sceneMgrCtx.pActiveScene != nullptr;
}

Scene* GetActiveScene()
{
    JAM_ASSERT(g_sceneMgrCtx.pActiveScene, "No active scene.");
    return g_sceneMgrCtx.pActiveScene;
}

bool HasScene(const std::string_view _name)
{
    return g_sceneMgrCtx.sceneMap.contains(std::string(_name));
}

Scene* GetScene(std::string_view _name)
{
    auto iter = g_sceneMgrCtx.sceneMap.find(std::string(_name));
    JAM_ASSERT(iter != g_sceneMgrCtx.sceneMap.end(), "Scene with name '{}' does not exist.", _name);
    return iter->second.get();
}

const HashMap<std::string, std::unique_ptr<Scene>>& GetAllScenes()
{
    return g_sceneMgrCtx.sceneMap;
}
}   // namespace jam::scene_manager