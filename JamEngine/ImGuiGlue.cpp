#include "pch.h"

#include "ImGuiGlue.h"

#include "Application.h"
#include "Gfx.h"

namespace jam::imgui
{

void Init(const HWND _hwnd, ID3D11Device* _pd3dDevice, ID3D11DeviceContext* _pd3dDeviceContext)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_pd3dDevice, _pd3dDeviceContext);
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
    gfx::SetFrameBuffer(k_invalidHandle);
    gfx::Touch();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

}   // namespace jam::imgui