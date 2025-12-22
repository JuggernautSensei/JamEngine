#pragma once

namespace jam
{

class PopupDialog
{
public:
    void Show();
    void Open(StringView _title, StringView _message);
    void Open(StringView _title, std::function<void()>&& _fn);

private:
    bool                  m_bOpen = false;
    std::string           m_title = {};
    std::function<void()> m_func  = {};
};

}   // namespace jam
