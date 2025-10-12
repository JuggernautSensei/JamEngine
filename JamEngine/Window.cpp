#include "pch.h"

#include "Window.h"

#include "Application.h"
#include "WindowsUtilities.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jam
{

void Window::Init()
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
        wClassEx.hbrBackground = NULL;
        wClassEx.lpszMenuName  = nullptr;
        wClassEx.lpszClassName = k_windowClassName;
        wClassEx.hIconSm       = nullptr;

        if (RegisterClassExW(&wClassEx) == 0)
        {
            DWORD lastError = GetLastError();
            JAM_CRASH("Failed to register window class: {}", CreateComErrorMessage(lastError));
        }
    }

    // create window
    {
        m_hWnd = CreateWindowExW(0, k_windowClassName, k_windowTitleName, WS_OVERLAPPEDWINDOW, k_defaultPosX, k_defaultPosY, k_defaultWidth, k_defaultHeight, nullptr, nullptr, m_hInstance, nullptr);
        if (m_hWnd == NULL)
        {
            DWORD lastError = GetLastError();
            JAM_CRASH("Failed to create window: {}", CreateComErrorMessage(lastError));
        }

        ResizeClient(k_defaultWidth, k_defaultHeight);
    }
}

void Window::Shutdown()
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    DestroyWindow(m_hWnd);
    UnregisterClassW(k_windowClassName, m_hInstance);

    m_hWnd      = NULL;
    m_hInstance = NULL;
}
bool Window::PollEvents()
{
    if (PeekMessage(&m_msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&m_msg);
        DispatchMessage(&m_msg);
        return true;
    }

    return false;
}

void Window::ResizeClient(const Int32 _clientWidth, const Int32 _clientHeight)
{
    JAM_ASSERT(m_hWnd, "Window handle is null");

    DWORD wsFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    DWORD exFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));
    SetClient(_clientWidth, _clientHeight, m_clientPos.x, m_clientPos.y, wsFlags, exFlags);
}

void Window::MoveClient(const Int32 _clientLeftTopX, const Int32 _clientLeftTopY)
{
    JAM_ASSERT(m_hWnd, "Window handle is null");

    DWORD wsFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    DWORD exFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));
    SetClient(m_clientSize.width, m_clientSize.height, _clientLeftTopX, _clientLeftTopY, wsFlags, exFlags);
}

void Window::SetTitle(const std::string_view _name) const
{
    JAM_ASSERT(m_hWnd, "Window handle is null");
    JAM_ASSERT(m_hInstance, "Window instance handle is null");

    std::wstring title = ToUTF16(_name);
    ::SetWindowText(m_hWnd, title.c_str());
}

Resolution Window::GetClientSize() const
{
    return m_clientSize;
}

ScreenPos Window::GetClientPos() const
{
    return m_clientPos;
}

Resolution Window::GetFullScreenSize() const
{
    Int32 w = ::GetSystemMetrics(SM_CXSCREEN);
    Int32 h = ::GetSystemMetrics(SM_CYSCREEN);
    return { w, h };
}

HWND Window::GetPlatformHandle() const
{
    return m_hWnd;
}

HINSTANCE Window::GetPlatformInstance() const
{
    return m_hInstance;
}

void Window::OnEvent(Event& _event)
{
    HandleEvent<WindowResizeEvent>(_event, JAM_BIND_MEMBER_LISTENER(Window::OnWindowResize_));
    HandleEvent<WindowMaximizeEvent>(_event, JAM_BIND_MEMBER_LISTENER(Window::OnWindowMaximize_));
    HandleEvent<WindowMoveEvent>(_event, JAM_BIND_MEMBER_LISTENER(Window::OnWindowMove_));
}

void Window::OnWindowResize_(const WindowResizeEvent& _event)
{
    m_clientSize.width  = _event.GetWidth();
    m_clientSize.height = _event.GetHeight();
}

void Window::OnWindowMaximize_(const WindowMaximizeEvent& _event)
{
    m_clientSize.width  = _event.GetWidth();
    m_clientSize.height = _event.GetHeight();
}

void Window::OnWindowMove_(const WindowMoveEvent& _event)
{
    m_clientPos.x = _event.GetX();
    m_clientPos.y = _event.GetY();
}

LRESULT Window::WindowProc_(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    Application& app    = Application::GetInstance();
    LRESULT      result = 0;

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            WindowCloseEvent closeEvent;
            app.DispatchEvent(closeEvent);
        }
        break;

        case WM_SIZE:
        {
            const Int32 width  = LOWORD(lParam);
            const Int32 height = HIWORD(lParam);

            switch (wParam)
            {
                case SIZE_MINIMIZED:   // 윈도우가 최소화 되었을 때
                {
                    WindowMinimizeEvent event;
                    app.DispatchEvent(event);
                }
                break;
                case SIZE_MAXIMIZED:   // 윈도우가 최대화 되었을 때
                {
                    WindowMaximizeEvent event { width, height };
                    app.DispatchEvent(event);
                }
                break;
                case SIZE_RESTORED:   // 일반적인 케이스
                {
                    WindowResizeEvent event { width, height };
                    app.DispatchEvent(event);
                }
                break;
                    // 그외 처리하지 않음
            }
        }
        break;

        case WM_MOVE:
        {
            const Int32     x = LOWORD(lParam);
            const Int32     y = HIWORD(lParam);
            WindowMoveEvent moveEvent { x, y };
            app.DispatchEvent(moveEvent);
        }
        break;

        case WM_MOUSEWHEEL:
        {
            const Int32     delta = GET_WHEEL_DELTA_WPARAM(wParam);
            MouseWheelEvent wheelEvent { delta };
            app.DispatchEvent(wheelEvent);
        }
        break;

        case WM_LBUTTONDOWN:
        {
            constexpr int  vKey = VK_LBUTTON;
            MouseDownEvent mouseDownEvent { vKey };
            app.DispatchEvent(mouseDownEvent);
        }
        break;

        case WM_LBUTTONUP:
        {
            constexpr int vKey = VK_LBUTTON;
            MouseUpEvent  mouseUpEvent { vKey };
            app.DispatchEvent(mouseUpEvent);
        }
        break;

        case WM_RBUTTONDOWN:
        {
            constexpr int  vKey = VK_RBUTTON;
            MouseDownEvent mouseDownEvent { vKey };
            app.DispatchEvent(mouseDownEvent);
        }
        break;

        case WM_RBUTTONUP:
        {
            constexpr int vKey = VK_RBUTTON;
            MouseUpEvent  mouseUpEvent { vKey };
            app.DispatchEvent(mouseUpEvent);
        }
        break;

        case WM_MBUTTONDOWN:
        {
            constexpr int  vKey = VK_MBUTTON;
            MouseDownEvent mouseDownEvent { vKey };
            app.DispatchEvent(mouseDownEvent);
        }
        break;

        case WM_MBUTTONUP:
        {
            constexpr int vKey = VK_MBUTTON;
            MouseUpEvent  mouseUpEvent { vKey };
            app.DispatchEvent(mouseUpEvent);
        }
        break;

        case WM_MOUSEMOVE:
        {
            const Int32    x = LOWORD(lParam);
            const Int32    y = HIWORD(lParam);
            MouseMoveEvent mouseMoveEvent { x, y };
            app.DispatchEvent(mouseMoveEvent);
        }
        break;

        case WM_KEYDOWN:
        {
            const int    vKey = static_cast<int>(wParam);
            KeyDownEvent keyDownEvent { vKey };
            app.DispatchEvent(keyDownEvent);
        }
        break;

        case WM_KEYUP:
        {
            const int  vKey = static_cast<int>(wParam);
            KeyUpEvent keyUpEvent { vKey };
            app.DispatchEvent(keyUpEvent);
        }
        break;

        default:
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return result;
}

void Window::SetClient(const Int32 _clientWidth, const Int32 _clientHeight, const Int32 _clientLeftTopX, const Int32 _clientLeftTopY, const DWORD _wsFlags, const DWORD _exFlags)
{
    bool bHasMenu = ::GetMenu(m_hWnd) != NULL;
    RECT rect     = { 0, 0, _clientWidth, _clientHeight };
    ::AdjustWindowRectExForDpi(&rect, _wsFlags, bHasMenu, _exFlags, ::GetDpiForWindow(m_hWnd));

    const Int32 adjustedWidth  = rect.right - rect.left;
    const Int32 adjustedHeight = rect.bottom - rect.top;
    ::SetWindowPos(m_hWnd, NULL, _clientLeftTopX, _clientLeftTopY, adjustedWidth, adjustedHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    ::UpdateWindow(m_hWnd);
    ::ShowWindow(m_hWnd, SW_SHOW);

    m_clientPos  = { _clientLeftTopX, _clientLeftTopY };
    m_clientSize = { _clientWidth, _clientHeight };
}

}   // namespace jam
