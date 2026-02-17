#pragma once
#include "Events.h"

namespace Jam
{

enum class eWindowPropertyFlags : UInt32
{
    None        = 0,
    NoResizable = 1 << 0,
    TopMost     = 1 << 1,

    DisableMinimizeButton = 1 << 2,
    DisableMaximizeButton = 1 << 3,
    DisableCloseButton    = 1 << 4,

    Borderless = 1 << 5,
};

class WindowsPlatform
{
public:
    WindowsPlatform(const WindowsPlatform&)            = delete;
    WindowsPlatform& operator=(const WindowsPlatform&) = delete;
    WindowsPlatform(WindowsPlatform&&)                 = delete;
    WindowsPlatform& operator=(WindowsPlatform&&)      = delete;

    [[nodiscard]] static WindowsPlatform& GetInstance();

    void ResizeClient(Int32 _clientWidth, Int32 _clientHeight);
    void MoveClient(Int32 _clientLeftTopX, Int32 _clientLeftTopY);
    void AdjustClientRect(Int32 _clientLeftTopX, Int32 _clientLeftTopY, Int32 _clientWidth, Int32 _clientHeight);
    void SetTitle(StringView _name) const;

    void                                      SetWindowProperty(Flags<eWindowPropertyFlags> _flags);
    [[nodiscard]] Flags<eWindowPropertyFlags> GetWindowProperty() const;

    [[nodiscard]] Vec2i GetClientPos() const;
    [[nodiscard]] Vec2i GetClientResolution() const;
    [[nodiscard]] Vec2i GetFullScreenResolution() const;

    [[nodiscard]] HWND      GetPlatformHandle() const;
    [[nodiscard]] HINSTANCE GetPlatformInstance() const;

private:
    WindowsPlatform();
    ~WindowsPlatform();

    static void CreateInstance();
    static void DestroyInstance();

    void OnEvent(Event& _event);
    bool PollEvents();

    void AdjustClientRect_(Int32 _x, Int32 _y, Int32 _width, Int32 _height);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event);
    void OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event);
    void OnWindowMoveEvent_(const WindowMoveEvent& _event);

    HINSTANCE m_hInstance = NULL;
    HWND      m_hWnd      = NULL;
    MSG       m_msg       = {};

    Vec2i                       m_clientPos = { 0, 0 };
    Vec2i                       m_clientRes = { 0, 0 };
    Flags<eWindowPropertyFlags> m_flags     = eWindowPropertyFlags::None;

    constexpr static const wchar_t* k_winClassName     = L"JAM_ENGINE_APP_WINDOW_CLASS";
    constexpr static const wchar_t* k_winTitleName     = L"Jam Engine Application";
    constexpr static Int32          k_initClientWidth  = 800;
    constexpr static Int32          k_initClientHeight = 600;
    constexpr static Int32          k_initClientPosX   = 0;
    constexpr static Int32          k_initClientPosY   = 0;

    inline static WindowsPlatform* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_WIN Jam::WindowsPlatform::GetInstance()