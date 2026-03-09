#pragma once

namespace Jam
{

template<typename T>
class Reference
{
public:
    explicit Reference(T& _value)
        : m_pValue(&_value)
    {
    }

    [[nodiscard]] operator T&() const
    {
        JAM_ASSERT(m_pValue != nullptr, "Reference::operator T&() - Attempting to convert to value from an empty reference.");
        return *m_pValue;
    }

    [[nodiscard]] T& Get() const
    {
        JAM_ASSERT(m_pValue != nullptr, "Reference::Get() - Attempting to get value from an empty reference.");
        return *m_pValue;
    }

    [[nodiscard]] T* GetPtr() const
    {
        return m_pValue;
    }

private:
    T* m_pValue = nullptr;
};

}   // namespace Jam
