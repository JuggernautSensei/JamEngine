#include "pch.h"

#include "Application.h"

#include "ILayer.h"
#include "ImguiLayer.h"
#include "Input.h"
#include "Renderer.h"
#include "SceneLayer.h"
#include "WindowsUtilities.h"

namespace
{

void OnNewFailed()
{
    JAM_CRASH("Failed to create application instance. Please check your CreateApplication implementation.");
}

}   // namespace

namespace jam
{

Application* Application::s_instance = nullptr;

void Application::Create(const jam::CommandLineArguments& _args)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");

    // log initialize
    Log::Initialize();

    // create application instance
    s_instance = CreateApplication(_args);

    // application on create routine
    {
        // set base properties
        std::set_new_handler(OnNewFailed);
        std::locale::global(std::locale(""));

        // initialize window
        s_instance->m_window.Initialize();
        s_instance->m_window.SetTitle(s_instance->m_applicationName);

        // initialize renderer
        Renderer::Initialize();

        // initialize input
        Input::Initialize();

        // attach layer
        ILayer* pSceneLayer       = s_instance->PushBackLayer(std::make_unique<SceneLayer>());
        s_instance->m_pSceneLayer = static_cast<SceneLayer*>(pSceneLayer);
        s_instance->PushBackLayer(std::make_unique<ImguiLayer>(s_instance->m_window.GetPlatformHandle(), Renderer::GetDevice(), Renderer::GetDeviceContext()));
    }

    // create routine of child application
    s_instance->OnCreate();
}

void Application::Destroy()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(s_instance->m_bRunning == false, "Application is still running, cannot destroy instance");

    // destroy routine of child application
    s_instance->OnDestroy();

    // application on destroy routine
    {
        s_instance->m_window.Shutdown();
    }

    // destroy application instance
    delete s_instance;
    s_instance = nullptr;

    // log shutdown
    Log::Shutdown();
}

Application& Application::GetInstance()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    return *s_instance;
}

Application::Application(const ApplicationCreateInfo& _info)
    : m_applicationName(_info.applicationName)
    , m_workingDirectory(_info.workingDirectory)
    , m_bRunning(true)
{
}

int Application::Run()
{
    m_timer.Start();

    while (m_bRunning)
    {
        if (!m_window.PollEvents())
        {
            // update
            {
                float deltaSec = m_timer.Tick();
                for (const std::unique_ptr<ILayer>& layer: m_layers)
                {
                    layer->OnUpdate(deltaSec);
                }

                for (const std::unique_ptr<ILayer>& layer: m_layers)
                {
                    layer->OnFinalUpdate(deltaSec);
                }

                // end frame
                Input::Update();
                m_commandQueue.Execute();
            }

            // rendering
            {
                for (const std::unique_ptr<ILayer>& layer: m_layers)
                {
                    layer->OnBeginRender();
                }

                for (const std::unique_ptr<ILayer>& layer: m_layers)
                {
                    layer->OnRender();
                }

                for (const std::unique_ptr<ILayer>& layer: m_layers)
                {
                    layer->OnEndRender();
                }

                Renderer::Present(m_bVsync);
            }
        }
    }

    return 0;
}

void Application::Quit()
{
    JAM_ASSERT(m_bRunning, "Application is not running");
    m_bRunning = false;
}

void Application::SetVsync(const bool _bVsync)
{
    m_bVsync = _bVsync;
}

bool Application::IsVsync() const
{
    JAM_ASSERT(s_instance, "Application instance is null");
    return m_bVsync;
}

void Application::DispatchEvent(Event& _event)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetName());

    //Log::Debug("occurred event: {}", _event.ToString());

    // application event listener routine
    if (_event.GetHash() == WindowCloseEvent::k_staticHash)
    {
        m_bRunning = false;
    }

    // dispatch event to other modules
    m_window.OnEvent(_event);
    Input::OnEvent(_event);
    Renderer::OnEvent(_event);
    for (const std::unique_ptr<ILayer>& layer: m_layers)
    {
        layer->OnEvent(_event);
    }
}

void Application::SubmitCommand(const std::function<void()>& _command)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_command, "Command is null");

    // submit command to command queue
    m_commandQueue.Submit(_command);
}

ILayer* Application::PushBackLayer(std::unique_ptr<ILayer>&& _layer)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_layer, "Layer pointer is null");

    // check if layer already exists in the stack
    {
        UInt32     layerHash = _layer->GetHash();
        const auto it        = std::ranges::find_if(m_layers,
                                             [layerHash](const std::unique_ptr<ILayer>& layer)
                                             {
                                                 return layer->GetHash() == layerHash;
                                             });

        JAM_ASSERT(it == m_layers.end(), "Layer with hash {} already exists", _layer->GetName());
    }

    // attach layer before pushing it to the stack
    const auto it = m_layers.insert(m_layers.end(), std::move(_layer));
    return it->get();
}

ILayer* Application::PushFrontLayer(std::unique_ptr<ILayer>&& _pLayer)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_pLayer, "Layer pointer is null");

    // check if layer already exists in the stack
    {
        UInt32     layerHash = _pLayer->GetHash();
        const auto it        = std::ranges::find_if(m_layers,
                                             [layerHash](const std::unique_ptr<ILayer>& layer)
                                             {
                                                 return layer->GetHash() == layerHash;
                                             });

        JAM_ASSERT(it == m_layers.end(), "Layer with hash {} already exists", _pLayer->GetName());
    }

    // attach layer before pushing it to the stack
    const auto it = m_layers.insert(m_layers.begin(), std::move(_pLayer));
    return it->get();
}

void Application::RemoveLayer(ILayer* _pLayer)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_pLayer, "Layer pointer is null");

    const auto it = std::ranges::find_if(m_layers,
                                         [_pLayer](const std::unique_ptr<ILayer>& layer)
                                         {
                                             return layer->GetHash() == _pLayer->GetHash();
                                         });

    JAM_ASSERT(it != m_layers.end(), "Layer not found in the application");
    m_layers.erase(it);
}

ILayer* Application::GetLayer(UInt32 _layerHash) const
{
    JAM_ASSERT(s_instance, "Application instance is null");
    const auto it = std::ranges::find_if(m_layers,
                                         [_layerHash](const std::unique_ptr<ILayer>& layer)
                                         {
                                             return layer->GetHash() == _layerHash;
                                         });

    JAM_ASSERT(it != m_layers.end(), "Layer with hash {} not found", _layerHash);
    return it->get();
}

const Window& Application::GetWindow() const
{
    return m_window;
}

SceneLayer* Application::GetSceneLayer() const
{
    JAM_ASSERT(m_pSceneLayer, "Scene layer is not set");
    return m_pSceneLayer;
}

const TickTimer& Application::GetTimer() const
{
    return m_timer;
}

}   // namespace jam

jam::Application& GetApplication()
{
    return jam::Application::GetInstance();
}