#include "pch.h"

#include "WindowsPlatform.h"

#include "Application.h"
#include "Error.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jam
{

void WindowsPlatform::Init()
{
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    JAM_CHECK_COM(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

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
            JAM_CRASH("Failed to register window class: {}", ErrorMessageFrom(lastError));
        }
    }

    // create window
    {
        m_hWnd = CreateWindowExW(0, k_windowClassName, k_windowTitleName, WS_OVERLAPPEDWINDOW, k_defaultPosX, k_defaultPosY, k_defaultWidth, k_defaultHeight, nullptr, nullptr, m_hInstance, nullptr);
        if (m_hWnd == NULL)
        {
            DWORD lastError = GetLastError();
            JAM_CRASH("Failed to create window: {}", ErrorMessageFrom(lastError));
        }

        ResizeClient(k_defaultWidth, k_defaultHeight);
        MoveClient(k_defaultPosX, k_defaultPosY);
    }

    // etc
    DragAcceptFiles(m_hWnd, TRUE);

    // init window properties
    SetWindowProperty(eWindowPropertyFlags_None);
}

void WindowsPlatform::Shutdown()
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");
    JAM_ASSERT(m_hInstance, "WindowsPlatform instance handle is null");

    CoUninitialize();
    DestroyWindow(m_hWnd);
    UnregisterClassW(k_windowClassName, m_hInstance);

    m_hWnd      = NULL;
    m_hInstance = NULL;
}

bool WindowsPlatform::PollEvents()
{
    if (PeekMessage(&m_msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&m_msg);
        DispatchMessage(&m_msg);
        return true;
    }
    else
    {
        return false;
    }
}

void WindowsPlatform::ResizeClient(const Int32 _clientWidth, const Int32 _clientHeight)
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");

    const DWORD wsFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    const DWORD exFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));
    SetClient(_clientWidth, _clientHeight, m_clientPos.x, m_clientPos.y, wsFlags, exFlags);
}

void WindowsPlatform::MoveClient(const Int32 _clientLeftTopX, const Int32 _clientLeftTopY)
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");

    const DWORD wsFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    const DWORD exFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));
    SetClient(m_clientRes.x, m_clientRes.y, _clientLeftTopX, _clientLeftTopY, wsFlags, exFlags);
}

void WindowsPlatform::SetTitle(const std::string_view _name) const
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");
    JAM_ASSERT(m_hInstance, "WindowsPlatform instance handle is null");

    const std::wstring title = ToUTF16(_name);
    ::SetWindowText(m_hWnd, title.c_str());
}

void WindowsPlatform::SetWindowProperty(const eWindowPropertyFlags _flags)
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");

    if (m_flags != _flags)
    {
        m_flags = _flags;

        DWORD wsFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
        DWORD exFlags = NarrowCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));

        // basic style
        wsFlags &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_OVERLAPPED);
        exFlags &= ~WS_EX_TOPMOST;

        // Borderless
        if (_flags & eWindowPropertyFlags_Borderless)
        {
            wsFlags |= WS_POPUP;
        }
        else
        {
            wsFlags |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

            // NoResizable
            if (!(_flags & eWindowPropertyFlags_NoResizable))
                wsFlags |= WS_THICKFRAME | WS_MAXIMIZEBOX;

            // Minimize
            if (!(_flags & eWindowPropertyFlags_DisableMinimizeButton))
                wsFlags |= WS_MINIMIZEBOX;

            // Maximize
            if (!(_flags & eWindowPropertyFlags_DisableMaximizeButton))
                wsFlags |= WS_MAXIMIZEBOX;

            // Close
            if (_flags & eWindowPropertyFlags_DisableCloseButton)
            {
                // Close 버튼만 비활성화
                if (HMENU hMenu = ::GetSystemMenu(m_hWnd, FALSE))
                {
                    ::EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
                }
            }
            else
            {
                // Close 버튼 활성화
                if (HMENU hMenu = ::GetSystemMenu(m_hWnd, FALSE))
                {
                    ::EnableMenuItem(hMenu, SC_CLOSE, 0);
                }
            }
        }

        // TopMost
        if (_flags & eWindowPropertyFlags_TopMost)
            exFlags |= WS_EX_TOPMOST;

        // 스타일 적용
        ::SetWindowLongPtr(m_hWnd, GWL_STYLE, wsFlags);
        ::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exFlags);

        // TopMost 위치 설정
        const HWND hWndInsertAfter = (_flags & eWindowPropertyFlags_TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST;
        ::SetWindowPos(m_hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

        // 윈도우 업데이트
        ::UpdateWindow(m_hWnd);
    }
}

eWindowPropertyFlags WindowsPlatform::GetWindowProperty() const
{
    return m_flags;
}

Vec2i WindowsPlatform::GetClientResolution() const
{
    return m_clientRes;
}

Vec2i WindowsPlatform::GetClientPos() const
{
    return m_clientPos;
}

Vec2i WindowsPlatform::GetFullScreenResolution() const
{
    const Int32 w = ::GetSystemMetrics(SM_CXSCREEN);
    const Int32 h = ::GetSystemMetrics(SM_CYSCREEN);
    return { w, h };
}

HWND WindowsPlatform::GetPlatformHandle() const
{
    return m_hWnd;
}

HINSTANCE WindowsPlatform::GetPlatformInstance() const
{
    return m_hInstance;
}

void WindowsPlatform::OnEvent(Event& _event)
{
    HandleEvent<WindowResizedEvent>(_event, this, &WindowsPlatform::OnWindowResizedEvent_);
    HandleEvent<WindowMaximizedEvent>(_event, this, &WindowsPlatform::OnWindowMaximizeEvent_);
    HandleEvent<WindowMoveEvent>(_event, this, &WindowsPlatform::OnWindowMoveEvent_);
}

void WindowsPlatform::OnWindowResizedEvent_(const WindowResizedEvent& _event)
{
    m_clientRes.x = _event.GetWidth();
    m_clientRes.y = _event.GetHeight();
}

void WindowsPlatform::OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event)
{
    m_clientRes.x = _event.GetWidth();
    m_clientRes.y = _event.GetHeight();
}

void WindowsPlatform::OnWindowMoveEvent_(const WindowMoveEvent& _event)
{
    m_clientPos.x = _event.GetX();
    m_clientPos.y = _event.GetY();
}

LRESULT WindowsPlatform::WindowProc_(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    static bool s_bIsResizing = false;

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

        case WM_ENTERSIZEMOVE:
        {
            s_bIsResizing = true;
        }
        break;

        case WM_EXITSIZEMOVE:
        {
            s_bIsResizing = false;

            RECT rect;
            ::GetClientRect(hWnd, &rect);
            const Int32 width  = Max(rect.right - rect.left, 1);
            const Int32 height = Max(rect.bottom - rect.top, 1);

            WindowResizedEvent event { width, height };
            app.DispatchEvent(event);
        }
        break;

        case WM_SIZE:
        {
            const Int32 width  = Max(LOWORD(lParam), 1);
            const Int32 height = Max(HIWORD(lParam), 1);

            switch (wParam)
            {
                case SIZE_MINIMIZED:   // 윈도우가 최소화 되었을 때
                {
                    WindowMinimizedEvent event;
                    app.DispatchEvent(event);
                }
                break;

                case SIZE_MAXIMIZED:   // 윈도우가 최대화 되었을 때
                {
                    WindowMaximizedEvent event { width, height };
                    app.DispatchEvent(event);
                }
                break;

                case SIZE_RESTORED:
                {
                    if (!s_bIsResizing)
                    {
                        WindowResizedEvent event { width, height };
                        app.DispatchEvent(event);
                    }
                }
                break;

                default:
                    break;
                    // 그외 처리하지 않음
            }
        }
        break;

        case WM_MOVE:
        {
            const Int32 x = LOWORD(lParam);
            const Int32 y = HIWORD(lParam);

            WindowMoveEvent moveEvent { x, y };
            app.DispatchEvent(moveEvent);
        }
        break;

        case WM_MOUSEWHEEL:
        {
            const Int32 delta = GET_WHEEL_DELTA_WPARAM(wParam);

            MouseWheelEvent wheelEvent { delta };
            app.DispatchEvent(wheelEvent);
        }
        break;

        case WM_LBUTTONDOWN:
        {
            constexpr Int32 vKey = VK_LBUTTON;
            MouseDownEvent  event { vKey };
            app.DispatchEvent(event);
        }
        break;

        case WM_LBUTTONUP:
        {
            constexpr Int32 vKey = VK_LBUTTON;
            MouseUpEvent    mouseUpEvent { vKey };
            app.DispatchEvent(mouseUpEvent);
        }
        break;

        case WM_RBUTTONDOWN:
        {
            constexpr Int32 vKey = VK_RBUTTON;
            MouseDownEvent  mouseDownEvent { vKey };
            app.DispatchEvent(mouseDownEvent);
        }
        break;

        case WM_RBUTTONUP:
        {
            constexpr Int32 vKey = VK_RBUTTON;
            MouseUpEvent    mouseUpEvent { vKey };
            app.DispatchEvent(mouseUpEvent);
        }
        break;

        case WM_MBUTTONDOWN:
        {
            constexpr Int32 vKey = VK_MBUTTON;
            MouseDownEvent  mouseDownEvent { vKey };
            app.DispatchEvent(mouseDownEvent);
        }
        break;

        case WM_MBUTTONUP:
        {
            constexpr Int32 vKey = VK_MBUTTON;
            MouseUpEvent    mouseUpEvent { vKey };
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
            const Int32  vKey = static_cast<int>(wParam);
            KeyDownEvent keyDownEvent { vKey };
            app.DispatchEvent(keyDownEvent);
        }
        break;

        case WM_KEYUP:
        {
            const Int32 vKey = static_cast<int>(wParam);
            KeyUpEvent  keyUpEvent { vKey };
            app.DispatchEvent(keyUpEvent);
        }
        break;

        case WM_ACTIVATE:
        case WM_SETFOCUS:
        {
            WindowFocusedEvent focusEvent;
            app.DispatchEvent(focusEvent);
        }
        break;

        case WM_KILLFOCUS:
        {
            WindowLostFocusEvent lostFocusEvent;
            app.DispatchEvent(lostFocusEvent);
        }
        break;

        default:
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return result;
}

void WindowsPlatform::SetClient(const Int32 _clientWidth, const Int32 _clientHeight, const Int32 _clientLeftTopX, const Int32 _clientLeftTopY, const DWORD _wsFlags, const DWORD _exFlags)
{
    bool bHasMenu = ::GetMenu(m_hWnd) != NULL;

    // cli -> win
    RECT adjRect = { 0, 0, _clientWidth, _clientHeight };
    ::AdjustWindowRectExForDpi(&adjRect, _wsFlags, bHasMenu, _exFlags, ::GetDpiForWindow(m_hWnd));
    const Int32 windowWidth  = adjRect.right - adjRect.left;
    const Int32 windowHeight = adjRect.bottom - adjRect.top;

    RECT curWinRect;
    ::GetWindowRect(m_hWnd, &curWinRect);

    // 오프셋 계산
    POINT clientPos = { 0, 0 };
    ::ClientToScreen(m_hWnd, &clientPos);
    const Int32 frameOffsetX = clientPos.x - curWinRect.left;
    const Int32 frameOffsetY = clientPos.y - curWinRect.top;

    // 윈도우 위치 계산
    const Int32 windowPosX = _clientLeftTopX - frameOffsetX;
    const Int32 windowPosY = _clientLeftTopY - frameOffsetY;

    ::SetWindowPos(m_hWnd, NULL, windowPosX, windowPosY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ::UpdateWindow(m_hWnd);
    ::ShowWindow(m_hWnd, SW_SHOW);

    m_clientPos = { _clientLeftTopX, _clientLeftTopY };
    m_clientRes = { _clientWidth, _clientHeight };
}

}   // namespace jam
