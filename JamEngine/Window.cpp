#include "pch.h"

#include "Window.h"

#include "Application.h"
#include "WindowsUtilities.h"
#include <nfd.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jam
{

void Window::Initialize()
{
    // add event listeners
    {
        m_eventDispatcher.AddListener<WindowMoveEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(Window::OnWindowMove));
        m_eventDispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(Window::OnWindowResize));
    }

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

std::vector<fs::path> Window::OpenFileDialog(const bool _bMultiSelectable, const std::span<const FileDialogFilter> _filters_orEmpty, const std::string_view _defaltPath_orEmpty) const
{
    // 필터 생성
    std::vector<nfdu8filteritem_t> filters;
    for (const FileDialogFilter& filter: _filters_orEmpty)
    {
        filters.emplace_back(filter.name.data(), filter.ext.data());
    }

    std::vector<fs::path> paths;
    nfdresult_t           result;

    if (_bMultiSelectable)
    {
        const nfdpathset_t* pOutPaths;
        result = NFD_OpenDialogMultiple(&pOutPaths, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPath_orEmpty.data());

        if (result == NFD_OKAY)
        {
            nfdpathsetsize_t count;
            NFD_PathSet_GetCount(pOutPaths, &count);

            for (nfdpathsetsize_t i = 0; i < count; ++i)
            {
                nfdu8char_t* nfdPath;
                NFD_PathSet_GetPath(pOutPaths, i, &nfdPath);
                paths.emplace_back(nfdPath);
                NFD_PathSet_FreePathU8(nfdPath);
            }
        }
    }
    else
    {
        nfdu8char_t* outPath = nullptr;
        result               = NFD_OpenDialog(&outPath,
                                filters.data(),
                                static_cast<nfdfiltersize_t>(filters.size()),
                                _defaltPath_orEmpty.data());

        if (result == NFD_OKAY)
        {
            paths.emplace_back(outPath);
            NFD_FreePathU8(outPath);
        }
        else if (result == NFD_ERROR)
        {
            JAM_ERROR("Failed to open file dialog: {}", NFD_GetError());
        }
    }

    return paths;
}

fs::path Window::SaveFileDialog(const std::span<const FileDialogFilter> _filters_orEmpty, const std::string_view _defaltPath_orEmpty) const
{
    // 필터 생성
    std::vector<nfdu8filteritem_t> filters;
    for (const FileDialogFilter& filter: _filters_orEmpty)
    {
        filters.emplace_back(filter.name.data(), filter.ext.data());
    }

    nfdu8char_t* pOutPath = nullptr;
    nfdresult_t  result   = NFD_SaveDialog(&pOutPath,
                                        filters.data(),
                                        static_cast<nfdfiltersize_t>(filters.size()),
                                        _defaltPath_orEmpty.data(),
                                        nullptr);

    fs::path outPath;
    if (result == NFD_OKAY)
    {
        outPath = pOutPath;
        NFD_FreePathU8(pOutPath);
    }
    else if (result == NFD_ERROR)
    {
        JAM_ERROR("Failed to save file dialog: {}", NFD_GetError());
    }
    return outPath;
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
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    LRESULT      result = 0;
    Application& app    = GetApplication();

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

            WindowResizeEvent resizeEvent { width, height };
            app.DispatchEvent(resizeEvent);
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
            const UInt32   x = LOWORD(lParam);
            const UInt32   y = HIWORD(lParam);
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

}   // namespace jam
