#pragma once
#include "Events.h"

namespace jam
{

enum WindowStyleFlags_ : Int32
{
    WindowStyleFlags_None        = 0,
    WindowStyleFlags_NoResize    = 1 << 0,   // resizeable window
    WindowStyleFlags_NoTitle     = 1 << 1,   // no title bar
    WindowStyleFlags_NoBorder    = 1 << 2,   // no border
    WindowStyleFlags_CenterAlign = 1 << 3,   // center align window
};
using WindowStyleFlags = Int32;   // window style flags

struct WindowInitializeInfo
{
    std::wstring     titleName  = L"jam engine window";
    Int32            width      = 1280;
    Int32            height     = 720;
    Int32            posX       = 100;
    Int32            posY       = 100;
    WindowStyleFlags styleFlags = WindowStyleFlags_None;
};

class Window
{
public:
    void Initialize(const WindowInitializeInfo& _info);
    void Shutdown();

    bool PollEvents();

    void ResizeWindow(Int32 _width, Int32 _height) const;
    void MoveWindow(Int32 _posX, Int32 _posY) const;

    void OnEvent(Event& _event) const;

    NODISCARD HWND      GetHandle() const { return m_hWnd; }
    NODISCARD HINSTANCE GetInstance() const { return m_hInstance; }

private:
    void SetWindowProperty(Int32 _width, Int32 _height, Int32 _posX, Int32 _posY, LONG _wsFlags) const;

    // event listener
    void OnWindowResize(const WindowResizeEvent& _event);
    void OnWindowMove(const WindowMoveEvent& _event);

    static WindowStyleFlags GetFlagsFromWindowStyle(LONG _style);
    static LONG             GetWindowStyleByFlags(WindowStyleFlags _flags);
    static LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
};

}   // namespace jam