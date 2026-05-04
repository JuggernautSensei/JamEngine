#pragma once

namespace Jam
{

template<typename T, typename Deleter = std::default_delete<T>>
class Scoped
{
public:
    using ValueT   = T;
    using DeleterT = Deleter;

    Scoped() noexcept
        : m_ptr(nullptr)
        , m_deleter()
    {
    }

    /* implicit */ Scoped(std::nullptr_t)
        : Scoped()
    {
    }

    /* implicit */ Scoped(T* _ptr)
        : m_ptr(_ptr)
        , m_deleter()
    {
    }

    /* implicit */ Scoped(T* _ptr, const Deleter& _deleter) noexcept
        : m_ptr(_ptr)
        , m_deleter(_deleter)
    {
    }

    /* implicit */ Scoped(T* _ptr, Deleter&& _deleter) noexcept
        : m_ptr(_ptr)
        , m_deleter(std::move(_deleter))
    {
    }

    ~Scoped()
    {
        Reset();
    }

    Scoped(const Scoped&) = delete;
    Scoped(Scoped&& _other) noexcept
        : m_ptr(_other.Release())
        , m_deleter(std::move(_other.m_deleter))
    {
    }

    template<typename U, typename E>
        requires(std::is_convertible_v<U*, T*> && std::is_convertible_v<E, Deleter>)
    /* implicit */ Scoped(Scoped<U, E>&& _other) noexcept
        : m_ptr(static_cast<T*>(_other.Release()))
        , m_deleter(std::forward<E>(_other.GetDeleter()))
    {
    }

    Scoped& operator=(const Scoped&) = delete;
    Scoped& operator=(Scoped&& _other) noexcept
    {
        if (this != &_other)
        {
            Reset(_other.Release());
            m_deleter = std::move(_other.m_deleter);
        }
        return *this;
    }

    template<typename U, typename E>
        requires(std::is_convertible_v<U*, T*> && std::is_convertible_v<E, Deleter>)
    Scoped& operator=(Scoped<U, E>&& _other) noexcept
    {
        Reset(static_cast<T*>(_other.Release()));
        m_deleter = std::forward<E>(_other.GetDeleter());
        return *this;
    }

    Scoped& operator=(std::nullptr_t)
    {
        Reset();
        return *this;
    }

    [[nodiscard]] T* GetPtr() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] T* Release() noexcept
    {
        T* p  = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    void Reset(T* _ptr = nullptr) noexcept
    {
        T* old = m_ptr;
        m_ptr  = _ptr;
        if (old)
        {
            m_deleter(old);
        }
    }

    void Reset(T* _ptr, const Deleter& _deleter) noexcept
    {
        if (m_ptr)
        {
            m_deleter(m_ptr);
        }

        m_ptr     = _ptr;
        m_deleter = _deleter;
    }

    void Reset(T* _ptr, Deleter&& _deleter) noexcept
    {
        if (m_ptr)
        {
            m_deleter(m_ptr);
        }

        m_ptr     = _ptr;
        m_deleter = std::move(_deleter);
    }

    [[nodiscard]] Deleter& GetDeleter() noexcept
    {
        return m_deleter;
    }

    [[nodiscard]] const Deleter& GetDeleter() const noexcept
    {
        return m_deleter;
    }

    void Swap(Scoped& _other) noexcept
    {
        using std::swap;
        swap(m_ptr, _other.m_ptr);
        swap(m_deleter, _other.m_deleter);
    }

    [[nodiscard]] T& operator*() const
    {
        return *m_ptr;
    }

    [[nodiscard]] T* operator->() const noexcept
    {
        return m_ptr;
    }

    explicit operator bool() const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return m_ptr == nullptr;
    }

    [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(const Scoped& _other) const noexcept
    {
        return m_ptr == _other.m_ptr;
    }

    [[nodiscard]] bool operator!=(const Scoped& _other) const noexcept
    {
        return m_ptr != _other.m_ptr;
    }

private:
    T*                            m_ptr     = nullptr;
    [[no_unique_address]] Deleter m_deleter = Deleter {};
};

template<typename T, typename Deleter>
class Scoped<T[], Deleter>
{
public:
    using ValueT   = T;
    using DeleterT = Deleter;

    Scoped() noexcept
        : m_ptr(nullptr)
        , m_deleter()
    {
    }

    /* implicit */ Scoped(std::nullptr_t)
        : Scoped()
    {
    }

    /* implicit */ Scoped(T* _ptr)
        : m_ptr(_ptr)
        , m_deleter()
    {
    }

    /* implicit */ Scoped(T* _ptr, const Deleter& _deleter) noexcept
        : m_ptr(_ptr)
        , m_deleter(_deleter)
    {
    }

    /* implicit */ Scoped(T* _ptr, Deleter&& _deleter) noexcept
        : m_ptr(_ptr)
        , m_deleter(std::move(_deleter))
    {
    }

    ~Scoped()
    {
        Reset();
    }

    Scoped(const Scoped&) = delete;
    Scoped(Scoped&& _other) noexcept
        : m_ptr(_other.Release())
        , m_deleter(std::move(_other.m_deleter))
    {
    }

    template<typename U, typename E>
        requires(std::is_convertible_v<U*, T*> && std::is_convertible_v<E, Deleter>)
    /* implicit */ Scoped(Scoped<U[], E>&& _other) noexcept
        : m_ptr(static_cast<T*>(_other.Release()))
        , m_deleter(std::forward<E>(_other.GetDeleter()))
    {
    }

    Scoped& operator=(const Scoped&) = delete;
    Scoped& operator=(Scoped&& _other) noexcept
    {
        if (this != &_other)
        {
            Reset(_other.Release());
            m_deleter = std::move(_other.m_deleter);
        }
        return *this;
    }

    template<typename U, typename E>
        requires(std::is_convertible_v<U*, T*> && std::is_convertible_v<E, Deleter>)
    Scoped& operator=(Scoped<U[], E>&& _other) noexcept
    {
        Reset(static_cast<T*>(_other.Release()));
        m_deleter = std::forward<E>(_other.GetDeleter());
        return *this;
    }

    Scoped& operator=(std::nullptr_t)
    {
        Reset();
        return *this;
    }

    [[nodiscard]] T* GetPtr() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] T* Release() noexcept
    {
        T* p  = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    void Reset(T* _ptr = nullptr) noexcept
    {
        T* old = m_ptr;
        m_ptr  = _ptr;
        if (old)
        {
            m_deleter(old);
        }
    }

    void Reset(T* _ptr, const Deleter& _deleter) noexcept
    {
        m_deleter = _deleter;
        Reset(_ptr);
    }

    void Reset(T* _ptr, Deleter&& _deleter) noexcept
    {
        m_deleter = std::move(_deleter);
        Reset(_ptr);
    }

    [[nodiscard]] Deleter& GetDeleter() noexcept
    {
        return m_deleter;
    }

    [[nodiscard]] const Deleter& GetDeleter() const noexcept
    {
        return m_deleter;
    }

    void Swap(Scoped& _other) noexcept
    {
        using std::swap;
        swap(m_ptr, _other.m_ptr);
        swap(m_deleter, _other.m_deleter);
    }

    [[nodiscard]] T& operator[](std::size_t _index) const
    {
        return m_ptr[_index];
    }

    explicit operator bool() const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return m_ptr == nullptr;
    }

    [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(const Scoped& _other) const noexcept
    {
        return m_ptr == _other.m_ptr;
    }

    [[nodiscard]] bool operator!=(const Scoped& _other) const noexcept
    {
        return m_ptr != _other.m_ptr;
    }

private:
    T*                            m_ptr     = nullptr;
    [[no_unique_address]] Deleter m_deleter = Deleter {};
};

template<typename T, typename... Args>
[[nodiscard]] Scoped<T> MakeScoped(Args&&... args)
{
    return Scoped<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
[[nodiscard]] Scoped<T[]> MakeScopedArray(
    const SizeT _size)
{
    return Scoped<T[]>(new T[_size]());
}

}   // namespace Jam

template<typename T, typename Deleter>
struct ankerl::unordered_dense::hash<Jam::Scoped<T, Deleter>>
{
    [[nodiscard]] std::size_t operator()(const Jam::Scoped<T, Deleter>& _str) const noexcept
    {
        return ankerl::unordered_dense::hash<std::size_t> {}(reinterpret_cast<std::size_t>(_str.GetPtr()));
    }
};

template<typename T, typename Deleter>
struct std::hash<Jam::Scoped<T, Deleter>>
{
    [[nodiscard]] std::size_t operator()(const Jam::Scoped<T, Deleter>& _str) const noexcept
    {
        return std::hash<std::size_t> {}(reinterpret_cast<std::size_t>(_str.GetPtr()));
    }
};
