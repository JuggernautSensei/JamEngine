#pragma once

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
        : m_ptr(nullptr)
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
    /* implicit */ Ref(U* _ptr)
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
        : m_ptr(static_cast<T*>(_ptr))
    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
    }

    template<typename U>
    /* implicit */ Ref(const Ref<U>& _other)
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
        : m_ptr(static_cast<T*>(_other.m_ptr))
    {
        if (m_ptr)
        {
            m_ptr->Retain();
        }
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
    Ref& operator=(const Ref<U>& _other)
        requires(std::is_convertible_v<U*, RefCounted*> && std::is_convertible_v<U*, T*>)
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

    void Reset()
    {
        Release_();
    }

    template<typename U>
    [[nodiscard]] Ref<U> As() const
        requires(std::is_base_of_v<RefCounted, U> && std::is_convertible_v<U*, T*>)
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
    [[nodiscard]] uint64_t operator()(const Jam::Ref<T>& value) const noexcept
    {
        return ankerl::unordered_dense::hash<T*> {}(value.GetPtr());
    }
};