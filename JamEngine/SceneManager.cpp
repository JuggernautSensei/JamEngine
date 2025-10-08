#include "pch.h"

#include "SceneManager.h"

#include "Application.h"
#include "Components.h"
#include "Scene.h"
#include "Script.h"
using namespace jam;

namespace
{

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

    Scene*            pActiveScene     = nullptr;
    Scene*            pNextScene       = nullptr;
    FrameBufferHandle sceneFrameBuffer = k_invalidHandle;
    TextureHandle     sceneTexture     = k_invalidHandle;
};
SceneMgrContext g_ctx;

}   // namespace

namespace jam
{
namespace scene_manager
{

    void Init()
    {
        Application&  app    = Application::GetInstance();
        const Window& win    = app.GetWindow();
        auto [width, height] = win.GetWindowSize();
        g_ctx.OnResize(width, height);
    }

    void Shutdown()
    {
        JAM_DESTROY_TEXTURE(g_ctx.sceneTexture);
        JAM_DESTROY_FRAME_BUFFER(g_ctx.sceneFrameBuffer);
    }

    void OnEvent(Event& _event)
    {
        HandleEvent<WindowResizeEvent>(_event, g_ctx.OnWindowResizeEvent);
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

            g_ctx.pActiveScene = g_ctx.pNextScene;
            g_ctx.pNextScene   = nullptr;

            if (g_ctx.pActiveScene)
            {
                g_ctx.pActiveScene->OnEnter();
            }
        }

        // update active scene
        if (g_ctx.pActiveScene)
        {
            // update scripts
            for (auto&& [_, scriptCmp]: g_ctx.pActiveScene->CreateView<ScriptComponent>().each())
            {
                if (scriptCmp.pScript)
                {
                    scriptCmp.pScript->Update(_deltaSec);
                }
            }

            // update scene
            g_ctx.pActiveScene->OnUpdate(_deltaSec);
        }
    }

    void RenderRuntime()
    {
        // find camera
        Camera*    pMainCamera    = nullptr;
        Transform* pMainTransform = nullptr;

        for (auto&& [_, cameraCmp, transformCmp]: g_ctx.pActiveScene->CreateView<CameraComponent, TransformComponent>().each())
        {
            if (cameraCmp.bMain)
            {
                pMainCamera    = &cameraCmp.camera;
                pMainTransform = &transformCmp.transform;
                break;
            }
        }

        if (pMainCamera && pMainTransform)
        {
            g_ctx.pActiveScene->OnRender(g_ctx.sceneFrameBuffer, *pMainCamera, *pMainTransform);
            gfx::BlitToBackBuffer(g_ctx.sceneTexture);   // 화면에 출력
        }
    }

    void Render(const FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform)
    {
        if (g_ctx.pActiveScene)
        {
            g_ctx.pActiveScene->OnRender(_handle, _camera, _transform);
        }
    }

    Scene* AddScene(std::string_view _name, std::unique_ptr<Scene>&& _pScene)
    {
        JAM_ASSERT(_pScene, "Null scene cannot be added.");
        JAM_ASSERT(!HasScene(_name), "Scene with name '{}' already exists.", _name);

        auto [iter, bResult] = g_ctx.sceneMap.emplace(std::string(_name), std::move(_pScene));
        JAM_ASSERT(bResult, "Failed to add scene with name '{}'.", _name);
        return iter->second.get();
    }

    void RemoveScene(std::string_view _name)
    {
        auto iter = g_ctx.sceneMap.find(std::string(_name));
        JAM_ASSERT(iter != g_ctx.sceneMap.end(), "Scene with name '{}' does not exist.", _name);
        JAM_ASSERT(iter->second.get() != g_ctx.pActiveScene, "Cannot remove the active scene.");
        g_ctx.sceneMap.erase(iter);
    }

    void ChangeScene(std::string_view _name)
    {
        auto iter = g_ctx.sceneMap.find(std::string(_name));
        JAM_ASSERT(iter != g_ctx.sceneMap.end(), "Scene with name '{}' does not exist.", _name);
        g_ctx.pNextScene = iter->second.get();
    }

    bool HasActiveScene()
    {
        return g_ctx.pActiveScene != nullptr;
    }

    Scene* GetActiveScene()
    {
        JAM_ASSERT(g_ctx.pActiveScene, "No active scene.");
        return g_ctx.pActiveScene;
    }

    bool HasScene(const std::string_view _name)
    {
        return g_ctx.sceneMap.contains(std::string(_name));
    }

    Scene* GetScene(std::string_view _name)
    {
        auto iter = g_ctx.sceneMap.find(std::string(_name));
        JAM_ASSERT(iter != g_ctx.sceneMap.end(), "Scene with name '{}' does not exist.", _name);
        return iter->second.get();
    }

    const HashMap<std::string, std::unique_ptr<Scene>>& GetAllScenes()
    {
        return g_ctx.sceneMap;
    }
}   // namespace scene_manager
}   // namespace jam