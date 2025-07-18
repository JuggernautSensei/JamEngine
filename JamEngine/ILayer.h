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

    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void OnUpdate(float _deltaTime) {}
    virtual void OnFixedUpdate(float _deltaTime) {}

    virtual void OnBeginRender() {}
    virtual void OnRender() {}
    virtual void OnRenderUI() {}
    virtual void OnEndRender() {}

    virtual void             OnEvent(Event& _event) {}
    virtual UInt32           GetHash() const = 0;
    virtual std::string_view GetName() const = 0;
};

}   // namespace jam