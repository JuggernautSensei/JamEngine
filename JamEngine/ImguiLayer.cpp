#include "pch.h"

#include "ImguiLayer.h"

#include "Events.h"
#include "Renderer.h"

namespace jam
{

ImguiLayer::ImguiLayer(const HWND _hwnd, ID3D11Device* _pDevice, ID3D11DeviceContext* _pDeviceContext)
{
    JAM_ASSERT(_pDevice, "Device pointer is null");
    JAM_ASSERT(_pDeviceContext, "Device context pointer is null");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_pDevice, _pDeviceContext);

    m_eventDispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(ImguiLayer::OnResize_));
    m_eventDispatcher.AddListener<BackBufferCleanupEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(ImguiLayer::OnBackBufferCleanup_));

    CreateScreenDependentResources_();
}

ImguiLayer::~ImguiLayer()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImguiLayer::OnBeginRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImguiLayer::OnEndRender()
{
    // back buffer binding
    m_backBufferTexture.BindAsRenderTarget();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImguiLayer::OnEvent(Event& _eventRef)
{
    m_eventDispatcher.Dispatch(_eventRef);
}

void ImguiLayer::CreateScreenDependentResources_()
{
    IDXGISwapChain* pSwapchain = Renderer::GetSwapchain();
    m_backBufferTexture.InitializeFromSwapchain(pSwapchain);
    m_backBufferTexture.AttachRTV();
}

void ImguiLayer::OnResize_(MAYBE_UNUSED const WindowResizeEvent& _event)
{
    CreateScreenDependentResources_();
}

void ImguiLayer::OnBackBufferCleanup_(MAYBE_UNUSED const BackBufferCleanupEvent& _event)
{
    m_backBufferTexture.Reset();
}

}   // namespace jam