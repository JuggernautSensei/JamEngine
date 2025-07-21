#pragma once
#include "ILayer.h"

namespace jam
{

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

    NODISCARD UInt32 GetHash() const override { return HashOf<ImguiLayer>(); }
    NODISCARD std::string_view GetName() const override { return NameOf<ImguiLayer>(); }
};

}   // namespace jam