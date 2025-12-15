#pragma once

namespace jam
{

// global error buffer
class ErrorBuffer
{
public:
    static void Init();
    static void Shutdown();

    static void Set(std::string_view _str);
    static void Clear();

    [[nodiscard]] static std::string_view Get();

    template<typename... Args>
    static void Set(const std::format_string<Args...> _fmt, Args&&... _args)
    {
        Set(std::format(_fmt, std::forward<Args>(_args)...));
    }
};

}   // namespace jam