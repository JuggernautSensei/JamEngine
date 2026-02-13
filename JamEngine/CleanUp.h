#pragma once

namespace Jam
{

template<typename TFunc>
class CleanUp
{
public:
    explicit constexpr CleanUp(TFunc&& _func)
        : m_func(std::move(_func))
    {
    }

    ~CleanUp()
    {
        m_func();
    }

    CleanUp(const CleanUp&)            = delete;
    CleanUp& operator=(const CleanUp&) = delete;
    CleanUp(CleanUp&&)                 = delete;
    CleanUp& operator=(CleanUp&&)      = delete;

private:
    [[no_unique_address]] TFunc m_func = {};
};

template<typename TFunc>
[[nodiscard]] CleanUp<TFunc> MakeCleanUp(
    TFunc&& _func)
{
    return CleanUp<TFunc>(std::forward<TFunc>(_func));
}

}   // namespace Jam