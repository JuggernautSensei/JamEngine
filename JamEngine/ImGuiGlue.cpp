#include "pch.h"

#include "ImGuiGlue.h"

#include "Graphics.h"

namespace jam
{

void ImGuiGlue::Init(const HWND _hwnd, ID3D11Device* _pd3dDevice, ID3D11DeviceContext* _pd3dDeviceContext)
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

void ImGuiGlue::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiGlue::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiGlue::EndFrame()
{
    Graphics::SetFrameBuffer(k_invalidHandle);
    Graphics::Flush();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

}   // namespace jam::imgui