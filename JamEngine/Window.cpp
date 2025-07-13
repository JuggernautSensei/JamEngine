#include "pch.h"

#include "Window.h"

#include "Application.h"
#include "WindowsUtilities.h"

namespace jam
{

void Window::Initialize(const WindowInitializeInfo& _info)
{
    m_hInstance = GetModuleHandle(NULL);

    // win class initialize
    WNDCLASSEXW wClassEx;
    wClassEx.cbSize        = sizeof(WNDCLASSEX);
    wClassEx.style         = CS_HREDRAW | CS_VREDRAW;
    wClassEx.lpfnWndProc   = WindowProc_;
    wClassEx.cbClsExtra    = 0;
    wClassEx.cbWndExtra    = 0;
    wClassEx.hInstance     = m_hInstance;
    wClassEx.hIcon         = nullptr;
    wClassEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wClassEx.hbrBackground = GetSysColorBrush(COLOR_WINDOW + 1);
    wClassEx.lpszMenuName  = nullptr;
    wClassEx.lpszClassName = k_windowClassName;
    wClassEx.hIconSm       = nullptr;

    if (RegisterClassExW(&wClassEx) == 0)
    {
        JAM_CRASH("Failed to register window class: {}", GetSystemLastErrorMessage());
    }

    // create window
    m_hWnd = CreateWindowExW(0, k_windowClassName, _info.titleName.c_str(), WS_OVERLAPPEDWINDOW, _info.posX, _info.posY, _info.width, _info.height, nullptr, nullptr, m_hInstance, nullptr);
    if (m_hWnd == NULL)
    {
        JAM_CRASH("Failed to create window: {}", GetSystemLastErrorMessage());
    }

    // add event listeners
    m_eventDispatcher.AddListener<WindowMoveEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(Window::OnWindowMove));
    m_eventDispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(Window::OnWindowResize));
}

void Window::Shutdown()
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    if (!DestroyWindow(m_hWnd))
    {
        JAM_CRASH("Failed to destroy window: {}", GetSystemLastErrorMessage());
    }

    if (!UnregisterClassW(k_windowClassName, m_hInstance))
    {
        JAM_CRASH("Failed to unregister window class: {}", GetSystemLastErrorMessage());
    }

    m_hWnd      = NULL;
    m_hInstance = NULL;
}

bool Window::PollEvents()
{
    if (PeekMessage(&m_message, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&m_message);
        DispatchMessage(&m_message);
        return true;
    }
    return false;
}

void Window::ResizeWindow(const Int32 _width, const Int32 _height) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    const LONG style      = GetWindowLongW(m_hWnd, GWL_STYLE);
    const bool bExistMenu = ::GetMenu(m_hWnd);

    RECT windowRect = { 0, 0, _width, _height };
    ::AdjustWindowRect(&windowRect, style, bExistMenu);

    const int width  = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;

    SetWindowProperty(width, height, windowRect.left, windowRect.top, GetFlagsFromWindowStyle(style));
}

void Window::MoveWindow(Int32 _posX, Int32 _posY) const
{
}

void Window::OnEvent(Event& _event) const
{
    m_eventDispatcher.Dispatch(_event);
}

void Window::SetWindowProperty(const Int32 _width, const Int32 _height, const Int32 _posX, const Int32 _posY, const LONG _wsFlags) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    SetWindowLongW(m_hWnd, GWL_STYLE, _wsFlags);
    SetWindowPos(m_hWnd, NULL, _posX, _posY, _width, _height, SWP_FRAMECHANGED | SWP_NOZORDER);

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
}

WindowStyleFlags Window::GetFlagsFromWindowStyle(const LONG _style)
{
    WindowStyleFlags flags = WindowStyleFlags_None;

    if ((_style & WS_THICKFRAME) == 0 && (_style & WS_MAXIMIZEBOX) == 0)
    {
        flags |= WindowStyleFlags_NoResize;
    }

    if ((_style & WS_CAPTION) == 0)
    {
        flags |= WindowStyleFlags_NoTitle;
    }

    if ((_style & WS_BORDER) == 0)
    {
        flags |= WindowStyleFlags_NoBorder;
    }

    return flags;
}

LONG Window::GetWindowStyleByFlags(const WindowStyleFlags _flags)
{
    LONG style = WS_OVERLAPPEDWINDOW;

    if (_flags & WindowStyleFlags_NoResize)
    {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    if (_flags & WindowStyleFlags_NoTitle)
    {
        style &= ~WS_CAPTION;
    }

    if (_flags & WindowStyleFlags_NoBorder)
    {
        style &= ~WS_BORDER;
    }

    return style;
}

void Window::OnWindowResize(const WindowResizeEvent& _event)
{
    m_width  = _event.GetWidth();
    m_height = _event.GetHeight();
}

void Window::OnWindowMove(const WindowMoveEvent& _event)
{
    m_posX = _event.GetX();
    m_posY = _event.GetY();
}

LRESULT Window::WindowProc_(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT      result = 0;
    Application& app    = GetApplication();

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;

        case WM_SIZE:
        {
            const Int32 width  = LOWORD(lParam);
            const Int32 height = HIWORD(lParam);

            WindowResizeEvent resizeEvent { width, height };
            app.DispatchEvent(resizeEvent);
        }
        break;

        default:
            result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return result;
}

}   // namespace jam
