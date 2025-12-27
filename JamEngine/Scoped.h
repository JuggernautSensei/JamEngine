#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace jam
{

template<typename T, typename Deleter = std::default_delete<T>>
class Scoped
{
public:
    Scoped() noexcept
        : m_ptr(nullptr)
        , m_deleter()
    {
    }

    /* implicit */ Scoped(std::nullptr_t)
        : Scoped()
    {
    }

    explicit Scoped(T* _ptr)
        : m_ptr(_ptr)
        , m_deleter()
    {
    }

    explicit Scoped(T* _ptr, const Deleter& _deleter) noexcept
        : m_ptr(_ptr)
        , m_deleter(_deleter)
    {
    }

    explicit Scoped(T* _ptr, Deleter&& _deleter) noexcept
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
        requires(std::is_convertible_v<U*, T*> && std::is_constructible_v<Deleter, E &&>)
    /* implicit */ Scoped(Scoped<U, E>&& _other) noexcept(std::is_nothrow_constructible_v<Deleter, E&&>)
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
        requires(std::is_convertible_v<U*, T*> && std::is_assignable_v<Deleter&, E &&>)
    Scoped& operator=(Scoped<U, E>&& _other) noexcept(std::is_nothrow_assignable_v<Deleter&, E&&>)
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

    void Reset(T* _ptr, const Deleter& _deleter) noexcept(std::is_nothrow_copy_assignable_v<Deleter>)
    {
        m_deleter = _deleter;
        Reset(_ptr);
    }

    void Reset(T* _ptr, Deleter&& _deleter) noexcept(std::is_nothrow_move_assignable_v<Deleter>)
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

    [[nodiscard]] T& operator*() const
    {
        return *m_ptr;
    }

    [[nodiscard]] T* operator->() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] explicit operator bool() const noexcept
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
    T* m_ptr = nullptr;

    [[no_unique_address]] Deleter m_deleter = Deleter();
};

template<typename T, typename... Args>
[[nodiscard]] Scoped<T> MakeScoped(Args&&... args)
{
    return Scoped<T>(new T(std::forward<Args>(args)...));
}

}   // namespace jam