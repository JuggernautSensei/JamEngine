#pragma once
#include "Events.h"

namespace jam
{

class Window
{
public:
    void Init();
    void Shutdown();
    void OnEvent(Event& _event);
    bool PollEvents();

    void ResizeWindow(Int32 _width, Int32 _height) const;
    void MoveWindow(Int32 _posX, Int32 _posY) const;
    void SetTitle(std::string_view _name) const;

    JAM_NODISCARD Resolution GetWindowSize() const;
    JAM_NODISCARD ScreenPos  GetWindowPos() const;
    JAM_NODISCARD Resolution GetMoniterScreenSize() const;
    JAM_NODISCARD HWND       GetPlatformHandle() const;
    JAM_NODISCARD HINSTANCE  GetPlatformInstance() const;

private:
    static LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetWindowAttribute_(Int32 _windowWidth, Int32 _windowHeight, Int32 _screenLeftTopX, Int32 _screenLeftTopY, LONG _wsFlags) const;
    void OnWindowResize_(const WindowResizeEvent& _event);
    void OnWindowMove_(const WindowMoveEvent& _event);

    HINSTANCE m_hInstance = NULL;   // 윈도우 인스턴스
    HWND      m_hWnd      = NULL;   // 윈도우 핸들
    MSG       m_msg       = {};     // 윈도우 메시지

    Int32 m_width  = 0;   // window width
    Int32 m_height = 0;   // window height
    Int32 m_posX   = 0;   // window translate X
    Int32 m_posY   = 0;   // window translate Y

    constexpr static const wchar_t* const k_windowClassName = L"jam enginewindow class";
    constexpr static const wchar_t* const k_windowTitleName = L"jam engine window";
    constexpr static Int32                k_defaultWidth    = 800;
    constexpr static Int32                k_defaultHeight   = 600;
    constexpr static Int32                k_defaultPosX     = 100;
    constexpr static Int32                k_defaultPosY     = 100;
};

}   // namespace jam