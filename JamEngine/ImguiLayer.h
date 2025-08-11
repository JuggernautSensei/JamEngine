#pragma once
#include "Event.h"
#include "ILayer.h"
#include "Textures.h"

namespace jam
{

class BackBufferCleanupEvent;
class WindowResizeEvent;

class ImguiLayer final : public ILayer
{
public:
    ImguiLayer(HWND _hwnd, ID3D11Device* _pDevice, ID3D11DeviceContext* _pDeviceContext);
    ~ImguiLayer() override;

    ImguiLayer(const ImguiLayer&)                = default;
    ImguiLayer& operator=(const ImguiLayer&)     = default;
    ImguiLayer(ImguiLayer&&) noexcept            = default;
    ImguiLayer& operator=(ImguiLayer&&) noexcept = default;

    void OnBeginRender() override;
    void OnEndRender() override;
    void OnEvent(Event& _eventRef) override;

    NODISCARD UInt32 GetHash() const override { return HashOf<ImguiLayer>(); }
    NODISCARD std::string_view GetName() const override { return NameOf<ImguiLayer>(); }

private:
    void CreateScreenDependentResources_();
    void OnResize_(const WindowResizeEvent& _event);
    void OnBackBufferCleanup_(const BackBufferCleanupEvent& _event);

    EventDispatcher m_eventDispatcher;
    Texture2D       m_backBufferTexture;
};

}   // namespace jam