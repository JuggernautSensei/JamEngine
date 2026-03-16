#pragma once
#include "MemoryAllocator.h"

namespace Jam::Tiny
{

template<typename T>
class Vector
{
public:
    Vector() = default;
    Vector(const Vector& _other)
        : m_data(nullptr)
        , m_size(_other.m_size)
        , m_cap(_other.m_cap)
    {
        if (m_cap > 0)
        {
            m_data = static_cast<T*>(Malloc(sizeof(T) * m_cap));
            for (SizeT i = 0; i < m_size; ++i)
            {
                std::construct_at(&m_data[i], _other.m_data[i]);
            }
        }
    }

    Vector& operator=(const Vector& _other)
    {
        if (this != &_other)
        {
            CleanUp_();
            m_size = _other.m_size;
            m_cap  = _other.m_cap;
            if (m_cap > 0)
            {
                m_data = static_cast<T*>(Malloc(sizeof(T) * m_cap));
                for (SizeT i = 0; i < m_size; ++i)
                {
                    std::construct_at(&m_data[i], _other.m_data[i]);
                }
            }
        }
        return *this;
    }

    Vector(Vector&& _other) noexcept
        : m_data(_other.m_data)
        , m_size(_other.m_size)
        , m_cap(_other.m_cap)
    {
        _other.m_data = nullptr;
        _other.m_size = 0;
        _other.m_cap  = 0;
    }

    Vector& operator=(Vector&& _other) noexcept
    {
        if (this != &_other)
        {
            CleanUp_();

            m_data        = _other.m_data;
            m_size        = _other.m_size;
            m_cap         = _other.m_cap;
            _other.m_data = nullptr;
            _other.m_size = 0;
            _other.m_cap  = 0;
        }
        return *this;
    }

    ~Vector()
    {
        CleanUp_();
    }

    template<typename U>
    void PushBack(U&& _value)
    {
        if (m_size >= m_cap)
        {
            Grow_(NextCap_(m_cap));
        }

        std::construct_at(&m_data[m_size], std::forward<U>(_value));
        ++m_size;
    }

    template<typename... TArgs>
    void EmplaceBack(TArgs&&... _args)
    {
        if (m_size >= m_cap)
        {
            Grow_(NextCap_(m_cap));
        }

        std::construct_at(&m_data[m_size], std::forward<TArgs>(_args)...);
        ++m_size;
    }

    void PopBack()
    {
        JAM_ASSERT(m_size > 0, "Vector is empty.");
        --m_size;
        std::destroy_at(&m_data[m_size]);
    }

    void Reserve(const SizeT _newCap)
    {
        if (_newCap > m_cap)
        {
            Grow_(_newCap);
        }
    }

    T& operator[](const SizeT _idx)
    {
        JAM_ASSERT(_idx >= 0 && _idx < m_size, "Index out of bound.");
        return m_data[_idx];
    }

    const T& operator[](const SizeT _idx) const
    {
        JAM_ASSERT(_idx >= 0 && _idx < m_size, "Index out of bound.");
        return m_data[_idx];
    }

    T& Back()
    {
        JAM_ASSERT(m_size > 0, "Vector is empty.");
        return m_data[m_size - 1];
    }

    const T& Back() const
    {
        JAM_ASSERT(m_size > 0, "Vector is empty.");
        return m_data[m_size - 1];
    }

    SizeT GetSize() const
    {
        return m_size;
    }

    void Clear()
    {
        std::destroy_n(m_data, m_size);
        m_size = 0;
    }

private:
    [[nodiscard]] SizeT NextCap_(const SizeT _curCap) const
    {
        return _curCap == 0 ? 4 : _curCap * 2;
    }

    void Grow_(const SizeT _newCap)
    {
        JAM_ASSERT(_newCap > m_cap, "New capacity must be greater.");

        T* pNewData = static_cast<T*>(Malloc(sizeof(T) * _newCap));
        for (SizeT i = 0; i < m_size; ++i)
        {
            std::construct_at(&pNewData[i], std::move(m_data[i]));
            std::destroy_at(&m_data[i]);
        }

        Free(m_data);
        m_data = pNewData;
        m_cap  = _newCap;
    }

    void CleanUp_()
    {
        Clear();
        if (m_data)
        {
            Free(m_data);
            m_data = nullptr;
        }
        m_cap = 0;
    }

    T*    m_data = nullptr;
    SizeT m_size = 0;
    SizeT m_cap  = 0;
};

}   // namespace Jam