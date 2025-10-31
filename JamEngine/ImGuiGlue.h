#pragma once
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace jam
{

class ImGuiGlue
{
public:

static void Init(HWND _hwnd);
static void Shutdown();
static void BeginFrame();
static void EndFrame();

};

}