#pragma once

namespace jam
{
class Event;

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

    virtual void OnBeginRender() {}
    virtual void OnRender() {}
    virtual void OnEndRender() {}

    virtual void             OnEvent(Event& _event) {}
    virtual NODISCARD UInt32 GetHash() const           = 0;
    virtual NODISCARD std::string_view GetName() const = 0;
};

}   // namespace jam