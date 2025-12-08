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

class WindowsPlatform
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

    [[nodiscard]] Resolution           GetClientResolution() const;
    [[nodiscard]] Vec2i                GetClientPos() const;
    [[nodiscard]] Resolution           GetFullScreenSize() const;
    [[nodiscard]] HWND                 GetPlatformHandle() const;
    [[nodiscard]] HINSTANCE            GetPlatformInstance() const;
    [[nodiscard]] eWindowPropertyFlags GetWindowProperty() const;

private:
    static LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetClient(Int32 _clientWidth, Int32 _clientHeight, Int32 _clientLeftTopX, Int32 _clientLeftTopY, DWORD _wsFlags, DWORD _exFlags);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event);
    void OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event);
    void OnWindowMoveEvent_(const WindowMoveEvent& _event);

    HINSTANCE m_hInstance = NULL;   // 윈도우 인스턴스
    HWND      m_hWnd      = NULL;   // 윈도우 핸들
    MSG       m_msg       = {};     // 윈도우 메시지

    Vec2i                m_clientPos  = { 0, 0 };
    Resolution           m_clientSize = { 0, 0 };
    eWindowPropertyFlags m_flags      = eWindowPropertyFlags_None;
};

}   // namespace jam