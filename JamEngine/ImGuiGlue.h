#pragma once
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_dx11.h"
#include "vendor/imgui/imgui_impl_win32.h"
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/imgui_stdlib.h"

namespace jam
{
namespace imgui
{

    void Init();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

}   // namespace imgui
}   // namespace jam