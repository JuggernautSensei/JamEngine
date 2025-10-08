#include "pch.h"

#include "ImGuiGlue.h"

#include "Application.h"
#include "Gfx.h"

namespace jam
{
namespace imgui
{

    void Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        const Application& app  = Application::GetInstance();
        const Window&      win  = app.GetWindow();
        const HWND         hwnd = win.GetPlatformHandle();
        JAM_ASSERT(hwnd, "Window handle is null");
        ImGui_ImplWin32_Init(hwnd);

        ID3D11Device*        pd3dDevice        = gfx::GetDevice();
        ID3D11DeviceContext* pd3dDeviceContext = gfx::GetDeviceContext();
        JAM_ASSERT(pd3dDevice, "Device pointer is null");
        JAM_ASSERT(pd3dDeviceContext, "Device context pointer is null");
        ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);
    }

    void Shutdown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void BeginFrame()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame()
    {
        gfx::SetFrameBuffer(k_invalidHandle); // back buffer

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

}   // namespace imgui
}   // namespace jam