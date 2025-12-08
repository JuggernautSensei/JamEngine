#pragma once

namespace jam
{

class PopupDialog
{
public:
    void Show();
    void Open(std::string_view _title, std::string_view _message);
    void Open(std::string_view _title, std::function<void()>&& _fn);

private:
    bool                  m_bOpen = false;
    std::string           m_title = {};
    std::function<void()> m_func  = {};
};

}   // namespace jam
