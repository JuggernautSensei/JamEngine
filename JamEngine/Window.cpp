#include "pch.h"

#include "Window.h"

#include "Application.h"
#include "WindowsUtilities.h"

namespace jam
{

void Window::Initialize()
{
    // dpi
    {
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    // check window class
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
    }

    // create window
    {
        m_hWnd = CreateWindowExW(0, k_windowClassName, k_windowTitleName, WS_OVERLAPPEDWINDOW, k_defaultPosX, k_defaultPosY, k_defaultWidth, k_defaultHeight, nullptr, nullptr, m_hInstance, nullptr);
        if (m_hWnd == NULL)
        {
            JAM_CRASH("Failed to create window: {}", GetSystemLastErrorMessage());
        }

        ShowWindow(m_hWnd, SW_SHOW);
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
    if (!PeekMessage(&m_message, nullptr, 0, 0, PM_REMOVE))
    {
        return false;
    }

    TranslateMessage(&m_message);
    DispatchMessage(&m_message);
    return true;
}

void Window::ResizeWindow(const Int32 _width, const Int32 _height) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    const LONG style      = GetWindowLongW(m_hWnd, GWL_STYLE);
    const bool bExistMenu = ::GetMenu(m_hWnd);

    // 클라이언트 영역을 _width, _height로 설정
    RECT windowRect = { 0, 0, _width, _height };
    ::AdjustWindowRect(&windowRect, style, bExistMenu);

    const int width  = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;

    SetWindowProperty_(width, height, m_posX, m_posY, style);
}
void Window::MoveWindow(const Int32 _posX, const Int32 _posY) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    const LONG style = GetWindowLongW(m_hWnd, GWL_STYLE);
    SetWindowProperty_(m_width, m_height, _posX, _posY, style);
}

void Window::SetTitle(const std::string_view _name) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    ::SetWindowText(m_hWnd, _name.data());
}

std::pair<Int32, Int32> Window::GetScreenSize() const
{
    Int32 w = ::GetSystemMetrics(SM_CXSCREEN);
    Int32 h = ::GetSystemMetrics(SM_CYSCREEN);
    return { w, h };
}

void Window::OnEvent(Event& _event) const
{
    m_eventDispatcher.Dispatch(_event);
}

void Window::SetWindowProperty_(const Int32 _width, const Int32 _height, const Int32 _posX, const Int32 _posY, const LONG _wsFlags) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    SetWindowLongW(m_hWnd, GWL_STYLE, _wsFlags);
    SetWindowPos(m_hWnd, NULL, _posX, _posY, _width, _height, SWP_FRAMECHANGED | SWP_NOZORDER);

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
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
    LRESULT result = 0;

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            Application& app = GetApplication();

            WindowCloseEvent closeEvent;
            app.DispatchEvent(closeEvent);
        }
        break;

        case WM_SIZE:
        {
            Application& app    = GetApplication();
            const Int32  width  = LOWORD(lParam);
            const Int32  height = HIWORD(lParam);

            WindowResizeEvent resizeEvent { width, height };
            app.DispatchEvent(resizeEvent);
        }
        break;

        case WM_MOVE:
        {
            Application& app = GetApplication();
            const Int32  x   = LOWORD(lParam);
            const Int32  y   = HIWORD(lParam);

            WindowMoveEvent moveEvent { x, y };
            app.DispatchEvent(moveEvent);
        }
        break;

        default:
            result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return result;
}

}   // namespace jam
