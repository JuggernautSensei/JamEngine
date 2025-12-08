#pragma once
#include "Vendor/imgui/Imgui.h"
#include "Vendor/imgui/Imgui_internal.h"
#include "Vendor/imgui/Imgui_stdlib.h"

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

}   // namespace jam