#pragma once
#include "Events.h"

namespace jam
{

class Window
{
public:
    void Initialize();
    void Shutdown();
    bool PollEvents();
    void OnEvent(Event& _event) const;

    void ResizeWindow(Int32 _width, Int32 _height) const;
    void MoveWindow(Int32 _posX, Int32 _posY) const;
    void SetTitle(std::string_view _name) const;

    NODISCARD std::pair<Int32, Int32> GetWindowSize() const { return { m_width, m_height }; }
    NODISCARD std::pair<Int32, Int32> GetWindowPosition() const { return { m_posX, m_posY }; }
    NODISCARD std::pair<Int32, Int32> GetScreenSize() const;
    NODISCARD HWND                    GetPlatformHandle() const { return m_hWnd; }
    NODISCARD HINSTANCE               GetPlatformInstance() const { return m_hInstance; }

private:
    // set window properties
    void SetWindowProperty_(Int32 _width, Int32 _height, Int32 _posX, Int32 _posY, LONG _wsFlags) const;

    // event listener
    void OnWindowResize(const WindowResizeEvent& _event);
    void OnWindowMove(const WindowMoveEvent& _event);

    HINSTANCE m_hInstance = NULL;   // instance handle
    HWND      m_hWnd      = NULL;   // window handle
    MSG       m_message   = {};     // message structure

    // cached
    Int32 m_width  = 0;   // window width
    Int32 m_height = 0;   // window height
    Int32 m_posX   = 0;   // window position X
    Int32 m_posY   = 0;   // window position Y

    EventDispatcher m_eventDispatcher;   // event dispatcher

    constexpr static const wchar_t* const k_windowClassName = L"jam_engine_window_class";   // window class name
    constexpr static const wchar_t* const k_windowTitleName = L"jam engine window";         // default window title name
    constexpr static Int32                k_defaultWidth    = 800;                          // default window width
    constexpr static Int32                k_defaultHeight   = 600;                          // default window height
    constexpr static Int32                k_defaultPosX     = 100;                          // default window position X
    constexpr static Int32                k_defaultPosY     = 100;                          // default window position Y

    static LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

}   // namespace jam