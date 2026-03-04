#pragma once
#include "ImGuiEngine.h"

namespace Jam
{

class Dialog : public RefCounted
{
public:
    explicit Dialog(StringView _title, ImVec2 _size = { 0.f, 0.f });

    void               Open();
    void               Show();
    [[nodiscard]] bool IsOpen() const;

protected:
    static constexpr float k_textWrapWidth = 400.f;

private:
    virtual void OnShow() = 0;
    bool         BeginPopup_() const;

    String m_title        = {};
    bool   m_bOpenRequest = true;
    bool   m_bOpen        = false;
    ImVec2 m_size         = {};
};

}   // namespace Jam
