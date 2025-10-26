#pragma once
#include "Events.h"

namespace jam
{

enum eWindowPropertyFlags_ : UInt32
{
    eWindowPropertyFlags_None        = 0,
    eWindowPropertyFlags_NoResizable = 1 << 0,
    eWindowPropertyFlags_TopMost     = 1 << 1,

    eWindowPropertyFlags_DisableMinimizeButton = 1 << 2,
    eWindowPropertyFlags_DisableMaximizeButton = 1 << 3,
    eWindowPropertyFlags_DisableCloseButton    = 1 << 4,

    eWindowPropertyFlags_Borderless = 1 << 5,
};
using eWindowPropertyFlags = UInt32;

class Window
{
public:
    void Init();
    void Shutdown();
    void OnEvent(Event& _event);
    bool PollEvents();

    void ResizeClient(Int32 _clientWidth, Int32 _clientHeight);
    void MoveClient(Int32 _clientLeftTopX, Int32 _clientLeftTopY);
    void SetTitle(std::string_view _name) const;
    void SetWindowProperty(eWindowPropertyFlags _flags);

    JAM_NODISCARD Resolution           GetClientResolution() const;
    JAM_NODISCARD ScreenPos            GetClientPos() const;
    JAM_NODISCARD Resolution           GetFullScreenSize() const;
    JAM_NODISCARD HWND                 GetPlatformHandle() const;
    JAM_NODISCARD HINSTANCE            GetPlatformInstance() const;
    JAM_NODISCARD eWindowPropertyFlags GetWindowProperty() const;

private:
    static LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetClient(Int32 _clientWidth, Int32 _clientHeight, Int32 _clientLeftTopX, Int32 _clientLeftTopY, DWORD _wsFlags, DWORD _exFlags);
    void OnWindowResize_(const WindowResizeEvent& _event);
    void OnWindowMaximize_(const WindowMaximizeEvent& _event);
    void OnWindowMove_(const WindowMoveEvent& _event);

    HINSTANCE m_hInstance = NULL;   // 윈도우 인스턴스
    HWND      m_hWnd      = NULL;   // 윈도우 핸들
    MSG       m_msg       = {};     // 윈도우 메시지

    ScreenPos            m_clientPos  = { 0, 0 };
    Resolution           m_clientSize = { k_defaultWidth, k_defaultHeight };
    eWindowPropertyFlags m_flags      = eWindowPropertyFlags_None;

    constexpr static const wchar_t* const k_windowClassName = L"jam enginewindow class";
    constexpr static const wchar_t* const k_windowTitleName = L"jam engine window";
    constexpr static Int32                k_defaultWidth    = 800;
    constexpr static Int32                k_defaultHeight   = 600;
    constexpr static Int32                k_defaultPosX     = 100;
    constexpr static Int32                k_defaultPosY     = 100;
};

}   // namespace jam