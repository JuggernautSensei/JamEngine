#pragma once
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace jam::imgui
{

void Init(HWND _hwnd, ID3D11Device* _pd3dDevice, ID3D11DeviceContext* _pd3dDeviceContext);
void Shutdown();
void BeginFrame();
void EndFrame();

}