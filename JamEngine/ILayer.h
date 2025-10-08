#pragma once

namespace jam
{

class Event;

/// @brief 레이어 인터페이스
class ILayer
{
public:
    ILayer()          = default;
    virtual ~ILayer() = default;

    ILayer(const ILayer&)                = default;
    ILayer& operator=(const ILayer&)     = default;
    ILayer(ILayer&&) noexcept            = default;
    ILayer& operator=(ILayer&&) noexcept = default;

    virtual void OnUpdate(float _deltaTime) {}
    virtual void OnFinalUpdate(float _deltaTime) {}
    virtual void OnRender() {}
    virtual void OnEvent(Event& _event) {}

    JAM_NODISCARD virtual std::string_view GetName() const = 0;
    JAM_NODISCARD virtual UInt32           GetHash() const = 0;
};

}   // namespace jam