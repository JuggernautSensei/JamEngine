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
#include "SceneNode.h"
#include "SceneSerializer.h"

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
    if (m_pActiveScene)
    {
        m_pActiveScene->OnResize(_width, _height);
    }
}

void SceneManager::Update(
    const float _deltaSec) const
{
    // update active scene
    if (m_pActiveScene)
    {
        // update scene
        m_pActiveScene->OnUpdate(_deltaSec);
    }
}

void SceneManager::RenderRuntime() const
{
    const auto [w, h] = JAM_GFX.GetBackBufferResolution();
    const Viewport vp = { NumericCast<UInt16>(w), NumericCast<UInt16>(h) };
    RenderRuntime(k_invalidHandle, vp);
}

void SceneManager::RenderRuntime(
    const FrameBufferHandle _fbh,
    const Viewport          _vp) const
{
    if (m_pActiveScene)
    {
        // update dirty transforms
        m_pActiveScene->UpdateSceneGraph_();

        // create view
        RenderView view = { _fbh, _vp };

        // find first enabled camera
        for (const auto& [_, sc, cc]: m_pActiveScene->Each<SceneComponent, CameraComponent>())
        {
            if (cc.bEnabled)
            {
                const Transform& transW = sc.node.GetWorldTransform();
                const Camera&    cmr    = cc.camera;

                view.eye  = transW.GetTranslation();
                view.view = transW.ToViewMatrix();
                view.proj = cmr.ToProjMatrix();
                break;
            }
        }

        // render
        m_pActiveScene->OnRender(view);
    }
}

void SceneManager::Render(
    const RenderView& _view) const
{
    if (m_pActiveScene)
    {
        m_pActiveScene->UpdateSceneGraph_();
        m_pActiveScene->OnRender(_view);
    }
}

void SceneManager::ImGuiRender() const
{
    if (const Ref<Scene>& pScene = m_pActiveScene)
    {
        pScene->OnImGuiRender();
    }
}

Error SceneManager::LoadScene(
    const Path& _path)
{
    Expect<JsonReader> reader = JsonReader::ParseFromFile(_path);
    if (!reader)
    {
        return reader.GetError();
    }

    Expect<Ref<Scene>> pScene = DeserializeScene(reader.GetValue());
    if (!pScene)
    {
        return pScene.GetError();
    }

    LoadScene(pScene.GetValue());
    return Error::None;
}

void SceneManager::LoadScene(
    const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "Null scene pointer");
    JAM_ASSERT(JAM_REFLECTION_DB.IsSceneReflected(_pScene->GetTypeHash()), "Scene type '{}' is not reflected. you must JAM_SCENE macro in the scene class definition.", _pScene->GetTypeFullName());
    ChangeSceneDefered_(_pScene);
}

bool SceneManager::HasActiveScene() const
{
    return m_pActiveScene != nullptr;
}

const Ref<Scene>& SceneManager::GetActiveScene() const
{
    JAM_ASSERT(m_pActiveScene, "No active scene");
    return m_pActiveScene;
}

SceneManager::~SceneManager()
{
    if (const Ref<Scene>& pScene = m_pActiveScene)
    {
        pScene->OnExit();
    }
}

void SceneManager::CreateInstance()
{
    JAM_ASSERT(!s_pInstance, "SceneManager already initialized");
    s_pInstance = new SceneManager;
}

void SceneManager::DestroyInstance()
{
    JAM_ASSERT(s_pInstance, "SceneManager not initialized");
    delete s_pInstance;
}

void SceneManager::OnEvent(
    Event& _event)
{
    if (!JAM_APP.IsEditorEnabled())   // runtime mode
    {
        // handle window events in runtime mode
        EventDispatcher dispatcher { _event };
        dispatcher.Dispatch<WindowResizedEvent>(this, &SceneManager::OnWindowResizedEvent_);
        dispatcher.Dispatch<WindowMaximizedEvent>(this, &SceneManager::OnWindowMaximizeEvent_);
    }

    m_pActiveScene->OnEvent(_event);
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
    const Ref<Scene>& _pScene)
{
    auto changeScene = [this, _pScene]
    {
        ChangeScene_(_pScene);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(changeScene)));
}

void SceneManager::ChangeScene_(
    const Ref<Scene>& _pScene)
{
    if (m_pActiveScene)
    {
        m_pActiveScene->OnExit();
    }

    m_pActiveScene = _pScene;
    m_pActiveScene->OnEnter();

    // resize in runtime.
    if (!JAM_APP.IsEditorEnabled())
    {
        const auto [w, h] = JAM_GFX.GetBackBufferResolution();
        Resize(w, h);
    }

    // resource garbage collection
    JAM_ASSET_MGR.CleanGarbageAll();

    // dispatch scene changed event
    SceneChangedEvent event = {};
    JAM_APP.DispatchEvent(event);
}

}   // namespace Jam