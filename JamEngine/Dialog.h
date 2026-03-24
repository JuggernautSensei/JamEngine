#pragma once
#include "ImGuiEngine.h"

namespace Jam
{

class Dialog : public RefCounted
{
public:
    explicit Dialog(StringView _title, ImVec2 _size = { 500.f, 0.f });

    void Open();
    bool BeginShow();
    void EndShow() const;

    [[nodiscard]] StringView GetTitle() const;

private:
    virtual void OnShow() = 0;
    bool         BeginPopup_() const;

    String m_title        = {};
    bool   m_bOpenRequest = true;
    ImVec2 m_initSize     = {};
};

}   // namespace Jam
