#pragma once
#include "Events.h"

namespace jam
{

class Window
{
public:
    void Init();
    void Shutdown();
    bool PollEvents();
    void OnEvent(Event& _event);

    // window
    void ResizeWindow(Int32 _width, Int32 _height) const;
    void MoveWindow(Int32 _posX, Int32 _posY) const;
    void SetTitle(std::string_view _name) const;

    JAM_NODISCARD Resolution GetWindowSize() const
    {
        return { m_width, m_height };
    }

    JAM_NODISCARD Resolution GetWindowPos() const
    {
        return { m_posX, m_posY };
    }

    JAM_NODISCARD Resolution GetMoniterScreenSize() const
    {
        Int32 w = ::GetSystemMetrics(SM_CXSCREEN);
        Int32 h = ::GetSystemMetrics(SM_CYSCREEN);
        return { w, h };
    }

    JAM_NODISCARD HWND GetPlatformHandle() const
    {
        return m_hWnd;
    }

    JAM_NODISCARD HINSTANCE GetPlatformInstance() const
    {
        return m_hInstance;
    }

private:
    void SetWindowAttribute_(
        Int32 _clientWidth,
        Int32 _clientHeight,
        Int32 _leftTopX,
        Int32 _leftTopY,
        LONG  _wsFlags) const;

    static LRESULT CALLBACK WindowProc_(
        HWND   hWnd,
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam);

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