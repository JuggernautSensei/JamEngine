#pragma once
#include "ImGuiGlue.h"

namespace Jam
{

enum class eDialogFlags : UInt32
{
    None       = 0,
    Modal      = 1 << 0,
    NoResize   = 1 << 1,
    NoMove     = 1 << 2,
    AutoResize = 1 << 3
};

struct DialogInfo
{
    StringView          title = {};
    ImVec2              size  = { 0.f, 0.f };
    Flags<eDialogFlags> flags = eDialogFlags::None;
};

class Dialog : public RefCounted
{
public:
    explicit Dialog(const DialogInfo& _desc);

    void               Open();
    void               Close();
    void               Show();
    [[nodiscard]] bool IsOpen() const;

protected:
    static constexpr float k_textWrapWidth = 400.f;

private:
    virtual void OnShow() = 0;
    bool         BeginPopup_() const;

    String              m_title        = {};
    bool                m_bOpenRequest = true;
    bool                m_bOpen        = false;
    ImVec2              m_size         = {};
    Flags<eDialogFlags> m_flags        = eDialogFlags::None;
};

}   // namespace Jam
