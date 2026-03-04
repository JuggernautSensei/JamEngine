#pragma once
#include "Scoped.h"

namespace Jam
{

class RefCounted
{
public:
    RefCounted()                             = default;
    virtual ~RefCounted()                    = default;
    RefCounted(const RefCounted&)            = default;
    RefCounted& operator=(const RefCounted&) = default;
    RefCounted(RefCounted&&)                 = default;
    RefCounted& operator=(RefCounted&&)      = default;

    void Retain()
    {
        ++m_refCount;
    }

    Int32 Release()
    {
        JAM_ASSERT(m_refCount > 0, "Reference count underflow");
        --m_refCount;
        return m_refCount;
    }

    [[nodiscard]] Int32 GetRefCount() const
    {
        return m_refCount;
    }

private:
    Int32 m_refCount = 0;
};

template<typename T>
class Ref
{
public:
    Ref()
        : m_ptr(nullptr)
    {
    }

    /* implicit */ Ref(std::nullptr_t)
        : Ref()
    {
    }

    /* implicit */ Ref(T* _ptr)
        : m_ptr(_ptr)

    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
    }

    /* implicit */ Ref(const Ref& _other)
        : m_ptr(_other.m_ptr)
    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
    }

    Ref(Ref&& _other) noexcept
        : m_ptr(_other.m_ptr)
    {
        _other.m_ptr = nullptr;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    /* implicit */ Ref(Scoped<U>&& _other)
        : m_ptr(static_cast<T*>(_other.Release()))
    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    /* implicit */ Ref(const Ref<U>& _other)
        : m_ptr(static_cast<T*>(_other.m_ptr))
    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    /* implicit */ Ref(Ref<U>&& _other) noexcept
        : m_ptr(static_cast<T*>(_other.m_ptr))
    {
        _other.m_ptr = nullptr;
    }

    ~Ref()
    {
        Release_();
    }

    Ref& operator=(const Ref& _other)
    {
        if (this != &_other)
        {
            if (_other.m_ptr)
            {
                _other.m_ptr->Retain();
            }
            Release_();
            m_ptr = _other.m_ptr;
        }
        return *this;
    }

    Ref& operator=(Ref&& _other) noexcept
    {
        if (this != &_other)
        {
            Release_();
            m_ptr        = _other.m_ptr;
            _other.m_ptr = nullptr;
        }
        return *this;
    }

    Ref& operator=(T* _ptr)
    {
        if (m_ptr != _ptr)
        {
            if (_ptr)
            {
                _ptr->Retain();
            }
            Release_();
            m_ptr = _ptr;
        }
        return *this;
    }

    Ref& operator=(std::nullptr_t)
    {
        Release_();
        return *this;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    Ref& operator=(Scoped<U>&& _other)
    {
        T* pOther = static_cast<T*>(_other.Release());
        JAM_ASSERT((m_ptr == nullptr && pOther == nullptr) || (m_ptr != pOther), "Cannot assign a Ref to a ScopedConnection if they don't point to the same object");

        if (pOther)
        {
            pOther->Retain();
        }
        Release_();
        m_ptr = pOther;
        return *this;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    Ref& operator=(const Ref<U>& _other)
    {
        T* pOther = static_cast<T*>(_other.m_ptr);
        if (m_ptr != pOther)
        {
            if (pOther)
            {
                pOther->Retain();
            }
            Release_();
            m_ptr = pOther;
        }
        return *this;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
    Ref& operator=(Ref<U>&& _other) noexcept
    {
        T* pOther = static_cast<T*>(_other.m_ptr);
        if (m_ptr != pOther)
        {
            Release_();
            m_ptr        = pOther;
            _other.m_ptr = nullptr;
        }
        else
        {
            _other.Release_();
        }
        return *this;
    }

    [[nodiscard]] T* operator->() const
    {
        return m_ptr;
    }

    [[nodiscard]] T& operator*() const
    {
        return *m_ptr;
    }

    [[nodiscard]] bool operator==(const T* _ptr) const
    {
        return m_ptr == _ptr;
    }

    [[nodiscard]] bool operator!=(const T* _ptr) const
    {
        return m_ptr != _ptr;
    }

    [[nodiscard]] bool operator==(const Ref& _other) const
    {
        return m_ptr == _other.m_ptr;
    }

    [[nodiscard]] bool operator!=(const Ref& _other) const
    {
        return m_ptr != _other.m_ptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const
    {
        return m_ptr == nullptr;
    }

    [[nodiscard]] bool operator!=(std::nullptr_t) const
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator<(const Ref& _other) const
    {
        return m_ptr < _other.m_ptr;
    }

    /* implicit */ operator bool() const
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] T* GetPtr() const
    {
        return m_ptr;
    }

    void Reset(T* _ptr = nullptr)
    {
        if (m_ptr != _ptr)
        {
            if (_ptr)
            {
                _ptr->Retain();
            }
            Release_();
            m_ptr = _ptr;
        }
    }

    template<typename U>
        requires(std::is_base_of_v<RefCounted, U> && std::is_convertible_v<U*, T*>)
    [[nodiscard]] Ref<U> As() const
    {
        return Ref<U>(static_cast<U*>(m_ptr));
    }

    template<typename U>
    friend class Ref;

private:
    void Release_()
    {
        if (m_ptr)
        {
            if (m_ptr->Release() == 0)
            {
                delete m_ptr;
            }
            m_ptr = nullptr;
        }
    }

    T* m_ptr = nullptr;
};

using AnyRef = Ref<RefCounted>;

template<typename T, typename... Args>
[[nodiscard]] Ref<T> MakeRef(Args&&... args)
{
    return Ref<T>(new T(std::forward<Args>(args)...));
}

}   // namespace Jam

template<typename T>
struct ankerl::unordered_dense::hash<Jam::Ref<T>>
{
    [[nodiscard]] std::size_t operator()(const Jam::Ref<T>& _ptr) const noexcept
    {
        return ankerl::unordered_dense::hash<std::size_t> {}(reinterpret_cast<std::size_t>(_ptr.GetPtr()));
    }
};

template<typename T>
struct std::hash<Jam::Ref<T>>
{
    [[nodiscard]] std::size_t operator()(const Jam::Ref<T>& _ptr) const noexcept
    {
        return std::hash<std::size_t> {}(reinterpret_cast<std::size_t>(_ptr.GetPtr()));
    }
};