#include "pch.h"

#include "WindowsPlatform.h"

#include <shellapi.h>

#include "Application.h"
#include "WindowsUtilities.h"
#include "Vendor/imgui/imgui.h"

// imgui win32 backend
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Jam
{

namespace
{
    LRESULT CALLBACK WindowProc(
        const HWND   hWnd,
        const UINT   uMsg,
        const WPARAM wParam,
        const LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        {
            return true;
        }

        static bool s_bUserGrabResizing = false;

        LRESULT result = 0;

        switch (uMsg)
        {
            case WM_DESTROY:
            {
                WindowCloseEvent closeEvent = {};
                GetApp().DispatchEvent(closeEvent);
            }
            break;

            case WM_ENTERSIZEMOVE:
            {
                s_bUserGrabResizing = true;
            }
            break;

            case WM_EXITSIZEMOVE:
            {
                s_bUserGrabResizing = false;

                RECT rect = {};
                ::GetClientRect(hWnd, &rect);
                const Int32 width  = Max(rect.right - rect.left, 1);
                const Int32 height = Max(rect.bottom - rect.top, 1);

                WindowResizedEvent event { width, height };
                GetApp().DispatchEvent(event);
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
                        GetApp().DispatchEvent(event);
                    }
                    break;

                    case SIZE_MAXIMIZED:   // 윈도우가 최대화 되었을 때
                    {
                        WindowMaximizedEvent event { width, height };
                        GetApp().DispatchEvent(event);
                    }
                    break;

                    case SIZE_RESTORED:
                    {
                        if (!s_bUserGrabResizing)   // grab으로 리사이즈하는 경우는 처리하지 않음.
                        {
                            WindowResizedEvent event { width, height };
                            GetApp().DispatchEvent(event);
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
                GetApp().DispatchEvent(moveEvent);
            }
            break;

            case WM_MOUSEWHEEL:
            {
                const Int32 delta = GET_WHEEL_DELTA_WPARAM(wParam);

                MouseWheelEvent wheelEvent { delta };
                GetApp().DispatchEvent(wheelEvent);
            }
            break;

            case WM_LBUTTONDOWN:
            {
                constexpr Int32 vKey = VK_LBUTTON;
                MouseDownEvent  event { vKey };
                GetApp().DispatchEvent(event);
            }
            break;

            case WM_LBUTTONUP:
            {
                constexpr Int32 vKey = VK_LBUTTON;
                MouseUpEvent    mouseUpEvent { vKey };
                GetApp().DispatchEvent(mouseUpEvent);
            }
            break;

            case WM_RBUTTONDOWN:
            {
                constexpr Int32 vKey = VK_RBUTTON;
                MouseDownEvent  mouseDownEvent { vKey };
                GetApp().DispatchEvent(mouseDownEvent);
            }
            break;

            case WM_RBUTTONUP:
            {
                constexpr Int32 vKey = VK_RBUTTON;
                MouseUpEvent    mouseUpEvent { vKey };
                GetApp().DispatchEvent(mouseUpEvent);
            }
            break;

            case WM_MBUTTONDOWN:
            {
                constexpr Int32 vKey = VK_MBUTTON;
                MouseDownEvent  mouseDownEvent { vKey };
                GetApp().DispatchEvent(mouseDownEvent);
            }
            break;

            case WM_MBUTTONUP:
            {
                constexpr Int32 vKey = VK_MBUTTON;
                MouseUpEvent    mouseUpEvent { vKey };
                GetApp().DispatchEvent(mouseUpEvent);
            }
            break;

            case WM_MOUSEMOVE:
            {
                const Int32    x = LOWORD(lParam);
                const Int32    y = HIWORD(lParam);
                MouseMoveEvent mouseMoveEvent { x, y };
                GetApp().DispatchEvent(mouseMoveEvent);
            }
            break;

            case WM_KEYDOWN:
            {
                const Int32  vKey = static_cast<int>(wParam);
                KeyDownEvent keyDownEvent { vKey };
                GetApp().DispatchEvent(keyDownEvent);
            }
            break;

            case WM_KEYUP:
            {
                const Int32 vKey = static_cast<int>(wParam);
                KeyUpEvent  keyUpEvent { vKey };
                GetApp().DispatchEvent(keyUpEvent);
            }
            break;

            case WM_ACTIVATE:
            case WM_SETFOCUS:
            {
                WindowFocusedEvent focusEvent = {};
                GetApp().DispatchEvent(focusEvent);
            }
            break;

            case WM_KILLFOCUS:
            {
                WindowLostFocusEvent lostFocusEvent = {};
                GetApp().DispatchEvent(lostFocusEvent);
            }
            break;

            default:
                result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        return result;
    }
}   // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WindowsPlatform& WindowsPlatform::GetInstance()
{
    JAM_ASSERT(s_pInstance, "WindowsPlatform instance is null. Call CreateInstance_() first.");
    return *s_pInstance;
}

void WindowsPlatform::ResizeClient(
    const Int32 _clientWidth,
    const Int32 _clientHeight)
{
    AdjustClientRect_(
        m_clientPos.x,
        m_clientPos.y,
        _clientWidth,
        _clientHeight);
}

void WindowsPlatform::MoveClient(
    const Int32 _clientLeftTopX,
    const Int32 _clientLeftTopY)
{
    AdjustClientRect_(
        _clientLeftTopX,
        _clientLeftTopY,
        m_clientRes.x,
        m_clientRes.y);
}

void WindowsPlatform::AdjustClientRect(
    const Int32 _clientLeftTopX,
    const Int32 _clientLeftTopY,
    const Int32 _clientWidth,
    const Int32 _clientHeight)
{
    AdjustClientRect_(
        _clientLeftTopX,
        _clientLeftTopY,
        _clientWidth,
        _clientHeight);
}

void WindowsPlatform::SetTitle(
    const StringView _name) const
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");
    JAM_ASSERT(m_hInstance, "WindowsPlatform instance handle is null");

    const WString name = WString::Encoded(_name);
    JAM_CHECK_WIN32(::SetWindowTextW(m_hWnd, name.GetPtr()));
}

void WindowsPlatform::SetWindowProperty(
    const Flags<eWindowPropertyFlags> _flags)
{
    JAM_ASSERT(m_hWnd, "WindowsPlatform handle is null");

    if (m_flags == _flags)
    {
        return;
    }

    m_flags = _flags;

    DWORD wsFlags = NumericCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    DWORD exFlags = NumericCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));

    // basic style
    wsFlags &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_OVERLAPPED);
    exFlags &= ~WS_EX_TOPMOST;

    // Borderless
    if (_flags & eWindowPropertyFlags::Borderless)
    {
        wsFlags |= WS_POPUP;
    }
    else
    {
        wsFlags |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

        // NoResizable
        if (!(_flags & eWindowPropertyFlags::NoResizable))
        {
            wsFlags |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }

        // Minimize
        if (!(_flags & eWindowPropertyFlags::DisableMinimizeButton))
        {
            wsFlags |= WS_MINIMIZEBOX;
        }

        // Maximize
        if (!(_flags & eWindowPropertyFlags::DisableMaximizeButton))
        {
            wsFlags |= WS_MAXIMIZEBOX;
        }

        // Close
        if (_flags & eWindowPropertyFlags::DisableCloseButton)
        {
            // Close 버튼만 비활성화
            if (const HMENU hMenu = GetSystemMenu(m_hWnd, FALSE))
            {
                JAM_CHECK_WIN32(::EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED));
            }
        }
        else
        {
            // Close 버튼 활성화
            if (const HMENU hMenu = GetSystemMenu(m_hWnd, FALSE))
            {
                JAM_CHECK_WIN32(::EnableMenuItem(hMenu, SC_CLOSE, 0));
            }
        }
    }

    // TopMost
    if (_flags & eWindowPropertyFlags::TopMost)
    {
        exFlags |= WS_EX_TOPMOST;
    }

    // 스타일 적용
    ::SetWindowLongPtr(m_hWnd, GWL_STYLE, wsFlags);
    ::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exFlags);

    // TopMost 위치 설정
    const HWND hWndInsertAfter = (_flags & eWindowPropertyFlags::TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST;
    JAM_CHECK_WIN32(::SetWindowPos(m_hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED));

    // 윈도우 업데이트
    JAM_CHECK_WIN32(::UpdateWindow(m_hWnd));
}

Flags<eWindowPropertyFlags> WindowsPlatform::GetWindowProperty() const
{
    return m_flags;
}

Vec2i WindowsPlatform::GetClientPos() const
{
    return m_clientPos;
}

Vec2i WindowsPlatform::GetClientResolution() const
{
    return m_clientRes;
}

Vec2i WindowsPlatform::GetFullScreenResolution() const
{
    const Int32 w = GetSystemMetrics(SM_CXSCREEN);
    const Int32 h = GetSystemMetrics(SM_CYSCREEN);
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

WindowsPlatform::WindowsPlatform()
{
    // set dpi awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // initialize com
    JAM_CHECK_HRESULT(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    // create window
    {
        m_hInstance = GetModuleHandleW(NULL);

        // win class initialize
        {
            WNDCLASSEX wClassEx    = {};
            wClassEx.cbSize        = sizeof(WNDCLASSEX);
            wClassEx.style         = CS_HREDRAW | CS_VREDRAW;
            wClassEx.lpfnWndProc   = WindowProc;
            wClassEx.cbClsExtra    = 0;
            wClassEx.cbWndExtra    = 0;
            wClassEx.hInstance     = m_hInstance;
            wClassEx.hIcon         = nullptr;
            wClassEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            wClassEx.hbrBackground = NULL;
            wClassEx.lpszMenuName  = nullptr;
            wClassEx.lpszClassName = k_winClassName;
            wClassEx.hIconSm       = nullptr;

            if (RegisterClassExW(&wClassEx) == 0)
            {
                JAM_CRASH("Failed to register window class: {}", CreateWindowsLastErrorMessage());
            }
        }

        // create window
        m_hWnd = CreateWindowExW(0, k_winClassName, k_winTitleName, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, m_hInstance, NULL);
        if (m_hWnd == NULL)
        {
            JAM_CRASH("Failed to create window: {}", CreateWindowsLastErrorMessage());
        }

        ::ShowWindow(m_hWnd, SW_SHOW);
        ::UpdateWindow(m_hWnd);
    }

    // etc
    DragAcceptFiles(m_hWnd, TRUE);

    // init window properties
    SetWindowProperty(eWindowPropertyFlags::None);
}

WindowsPlatform::~WindowsPlatform()
{
    CoUninitialize();
    DestroyWindow(m_hWnd);
    UnregisterClassW(k_winClassName, m_hInstance);

    m_hWnd      = NULL;
    m_hInstance = NULL;
}

void WindowsPlatform::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "WindowsPlatform already initialized");
    s_pInstance = new WindowsPlatform;
}

void WindowsPlatform::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

void WindowsPlatform::OnEvent(Event& _event)
{
    EventDispatcher dispatch { _event };
    dispatch.Dispatch<WindowResizedEvent>(this, &WindowsPlatform::OnWindowResizedEvent_);
    dispatch.Dispatch<WindowMaximizedEvent>(this, &WindowsPlatform::OnWindowMaximizeEvent_);
    dispatch.Dispatch<WindowMoveEvent>(this, &WindowsPlatform::OnWindowMoveEvent_);
}

bool WindowsPlatform::PollEvents()
{
    if (!PeekMessage(&m_msg, nullptr, 0, 0, PM_REMOVE))
    {
        return false;
    }

    TranslateMessage(&m_msg);
    DispatchMessage(&m_msg);
    return true;
}

void WindowsPlatform::AdjustClientRect_(
    const Int32 _x,
    const Int32 _y,
    const Int32 _width,
    const Int32 _height)
{
    const bool  bHasMenu = (GetMenu(m_hWnd) != NULL);
    const DWORD wsFlags  = NumericCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_STYLE));
    const DWORD exFlags  = NumericCast<DWORD>(::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE));

    // cli -> win
    RECT adjRect = { _x, _y, _x + _width, _y + _height };
    JAM_CHECK_WIN32(::AdjustWindowRectExForDpi(&adjRect, wsFlags, bHasMenu, exFlags, ::GetDpiForWindow(m_hWnd)));
    const Int32 windowWidth  = adjRect.right - adjRect.left;
    const Int32 windowHeight = adjRect.bottom - adjRect.top;

    RECT curWinRect = {};
    JAM_CHECK_WIN32(::GetWindowRect(m_hWnd, &curWinRect));

    // 오프셋 계산
    POINT clientPos = {};
    JAM_CHECK_WIN32(::ClientToScreen(m_hWnd, &clientPos));
    const Int32 frameOffsetX = clientPos.x - curWinRect.left;
    const Int32 frameOffsetY = clientPos.y - curWinRect.top;

    // 윈도우 위치 계산
    const Int32 windowPosX = _x - frameOffsetX;
    const Int32 windowPosY = _y - frameOffsetY;

    JAM_CHECK_WIN32(::SetWindowPos(m_hWnd, NULL, windowPosX, windowPosY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED));
    JAM_CHECK_WIN32(::UpdateWindow(m_hWnd));
    JAM_CHECK_WIN32(::ShowWindow(m_hWnd, SW_SHOW));

    m_clientPos = { _x, _y };
    m_clientRes = { _width, _height };
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

}   // namespace Jam
