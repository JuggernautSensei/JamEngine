#pragma once

namespace jam
{

// interface
class IModalBoxPanel
{
public:
    virtual ~IModalBoxPanel() = default;

    IModalBoxPanel()                                  = default;
    IModalBoxPanel(const IModalBoxPanel&)                = default;
    IModalBoxPanel& operator=(const IModalBoxPanel&)     = default;
    IModalBoxPanel(IModalBoxPanel&&) noexcept            = default;
    IModalBoxPanel& operator=(IModalBoxPanel&&) noexcept = default;

    virtual void                       Show()           = 0;
    JAM_NODISCARD virtual StringView GetTitle() const = 0;
};

}   // namespace jam
