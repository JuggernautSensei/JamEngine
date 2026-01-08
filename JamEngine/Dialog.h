#pragma once

namespace jam
{

enum eDialogFlags_
{
    eDialogFlags_None     = 0,
    eDialogFlags_Modal    = 1 << 0,
    eDialogFlags_NoResize = 1 << 1,
    eDialogFlags_NoMove   = 1 << 2,
};
using eDialogFlags = UnderlyingType<eDialogFlags_>;

class Dialog
{
public:
    // (0.f, 0.f) << auto size
    explicit Dialog(StringView _title, ImVec2 _size = ImVec2 { 0.f, 0.f }, eDialogFlags _flags = eDialogFlags_None);
    virtual ~Dialog() = default;

    Dialog(const Dialog&)            = default;
    Dialog(Dialog&&)                 = default;
    Dialog& operator=(const Dialog&) = default;
    Dialog& operator=(Dialog&&)      = default;

    void               Open();
    void               Close();
    void               Show();
    [[nodiscard]] bool IsOpen() const;

protected:
    static constexpr float k_textWrapWidth = 400.f;

private:
    virtual void OnShow() = 0;
    bool         BeginPopup_() const;

    std::string  m_title       = {};
    bool         m_bOpenRequest     = false;
    bool         m_bOpen       = false;
    ImVec2       m_size        = {};
    bool         m_bAutoResize = false;
    eDialogFlags m_flags       = eDialogFlags_None;
};

}   // namespace jam
