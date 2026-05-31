#pragma once
#include <type_traits>
#include "FunctionTraits.h"
#include "FunctionHasher.h"

namespace Jam
{

template<typename>
class Delegate;

template<typename TRet, typename... TArg>
class Delegate<TRet(TArg...)>
{
    using TFunc    = TRet (*)(const Delegate*, TArg...);
    using TDeleter = void (*)(Delegate*);
    using TCloner  = void (*)(Delegate*, const Delegate*);

    static constexpr Int32 kSBOSize  = sizeof(void*) * 2;
    static constexpr Int32 kSBOAlign = alignof(void*);

    struct alignas(kSBOAlign) SBO
    {
        Array<Byte, kSBOSize> data = {};
    };

public:
    Delegate() = default;

    /* implicit */ Delegate(std::nullptr_t)
    {
    }

    ~Delegate()
    {
        Disconnect();
    }

    Delegate(const Delegate& _other)
        : m_func(_other.m_func)
        , m_deleter(_other.m_deleter)
        , m_copier(_other.m_copier)
        , m_hash(_other.m_hash)
    {
        if (m_copier)
        {
            m_copier(this, &_other);
        }
        else
        {
            m_storage = _other.m_storage;
        }
    }

    Delegate(Delegate&& _other) noexcept
        : m_func(_other.m_func)
        , m_deleter(_other.m_deleter)
        , m_copier(_other.m_copier)
        , m_hash(_other.m_hash)
    {
        m_storage        = _other.m_storage;
        _other.m_func    = nullptr;
        _other.m_deleter = nullptr;
        _other.m_copier  = nullptr;
        _other.m_empty   = {};
        _other.m_hash    = 0;
    }

    Delegate& operator=(std::nullptr_t)
    {
        Disconnect();
        return *this;
    }

    Delegate& operator=(const Delegate& _other)
    {
        if (this != &_other)
        {
            Disconnect();
            m_hash    = _other.m_hash;
            m_func    = _other.m_func;
            m_deleter = _other.m_deleter;
            m_copier  = _other.m_copier;
            if (m_copier)
            {
                m_copier(this, &_other);
            }
            else
            {
                m_storage = _other.m_storage;
            }
        }
        return *this;
    }

    Delegate& operator=(Delegate&& _other) noexcept
    {
        if (this != &_other)
        {
            Disconnect();
            m_func           = _other.m_func;
            m_deleter        = _other.m_deleter;
            m_copier         = _other.m_copier;
            m_storage        = _other.m_storage;
            m_hash           = _other.m_hash;
            _other.m_func    = nullptr;
            _other.m_deleter = nullptr;
            _other.m_copier  = nullptr;
            _other.m_empty   = {};
            _other.m_hash    = 0;
        }
        return *this;
    }

    template<auto TFunc>
        requires std::is_invocable_r_v<TRet, decltype(TFunc), TArg...>
    void Connect()   // for free function or static member function or non-capturing lambda
    {
        Disconnect();
        m_hash = FuncHashOf<TFunc>();
        m_func = [](const Delegate*, TArg... args) -> TRet {
            return TFunc(std::forward<TArg>(args)...);
        };
    }

    template<auto TMethod, typename TCaller>
        requires std::is_invocable_r_v<TRet, decltype(TMethod), TCaller*, TArg...>
    void Connect(TCaller* _pInst)   // for non-const member function
    {
        Disconnect();
        m_hash  = FuncHashOf<TMethod>(_pInst);
        m_pInst = static_cast<void*>(_pInst);
        m_func  = [](const Delegate* _pDel, TArg... args) -> TRet {
            TCaller* pCaller = static_cast<TCaller*>(_pDel->m_pInst);
            return (pCaller->*TMethod)(std::forward<TArg>(args)...);
        };
    }

    template<auto TMethod, typename TCaller>
        requires std::is_invocable_r_v<TRet, decltype(TMethod), const TCaller*, TArg...>
    void Connect(const TCaller* _pInst)   // for const member function
    {
        Disconnect();
        m_hash       = FuncHashOf<TMethod>(_pInst);
        m_pConstInst = static_cast<const void*>(_pInst);
        m_func       = [](const Delegate* _pDel, TArg... args) -> TRet {
            const TCaller* pCaller = static_cast<const TCaller*>(_pDel->m_pConstInst);
            return (pCaller->*TMethod)(std::forward<TArg>(args)...);
        };
    }

    template<typename TCallable>
    void Connect(TCallable&& _func)   // for functor. capturing lambda ...
    {
        using TDecayed = std::decay_t<TCallable>;

        Disconnect();
        m_hash = FuncHashOf(_func);

        if constexpr (sizeof(TDecayed) <= kSBOSize && alignof(TDecayed) <= kSBOAlign)
        {
            // Small buffer optimization

            new (&m_storage) TDecayed(std::forward<TCallable>(_func));

            m_func = [](const Delegate* _pDel, TArg... args) -> TRet {
                const TDecayed& func = *reinterpret_cast<const TDecayed*>(&_pDel->m_storage);
                return func(std::forward<TArg>(args)...);
            };

            m_deleter = [](Delegate* _pDel) {
                TDecayed& func = *reinterpret_cast<TDecayed*>(&_pDel->m_storage);
                func.~TDecayed();
            };

            m_copier = [](Delegate* _pDst, const Delegate* _pSrc) {
                const TDecayed& srcFunc = *reinterpret_cast<const TDecayed*>(&_pSrc->m_storage);
                new (&_pDst->m_storage) TDecayed(srcFunc);
            };
        }
        else
        {
            struct FuncHolder : RefCounted
            {
                explicit FuncHolder(TDecayed&& _f)
                    : func(std::forward<TDecayed>(_f))
                {
                }

                TDecayed func;
            };

            new (&m_pRef) AnyRef(new FuncHolder(std::forward<TCallable>(_func)));

            // ReSharper disable once CppParameterMayBeConstPtrOrRef
            m_func = [](const Delegate* _pDel, TArg... args) -> TRet {
                const FuncHolder* pHolder = static_cast<const FuncHolder*>(_pDel->m_pRef.GetPtr());
                return pHolder->func(std::forward<TArg>(args)...);
            };

            m_deleter = [](Delegate* _pDel) {
                _pDel->m_pRef.~AnyRef();
            };

            m_copier = [](Delegate* _pDst, const Delegate* _pSrc) {
                new (&_pDst->m_pRef) AnyRef(_pSrc->m_pRef);
            };
        }
    }

    TRet Call(TArg... args) const
    {
        JAM_ASSERT(m_func != nullptr, "Delegate not connected");
        return m_func(this, std::forward<TArg>(args)...);
    }

    TRet operator()(TArg... args) const
    {
        JAM_ASSERT(m_func != nullptr, "Delegate not connected");
        return m_func(this, std::forward<TArg>(args)...);
    }

    TRet Call(TArg... args)
    {
        JAM_ASSERT(m_func != nullptr, "Delegate not connected");
        return m_func(this, std::forward<TArg>(args)...);
    }

    TRet operator()(TArg... args)
    {
        JAM_ASSERT(m_func != nullptr, "Delegate not connected");
        return m_func(this, std::forward<TArg>(args)...);
    }

    explicit operator bool() const
    {
        return IsConnected();
    }

    [[nodiscard]] bool IsConnected() const
    {
        return m_func != nullptr;
    }

    void Disconnect()
    {
        if (m_deleter)
        {
            m_deleter(this);
        }

        m_deleter = nullptr;
        m_copier  = nullptr;
        m_empty   = {};
        m_func    = nullptr;
        m_hash    = 0;
    }

    [[nodiscard]] UInt64 GetHash() const
    {
        return m_hash;
    }

    [[nodiscard]] UInt64 GetCallerAddress() const
    {
        if (!IsMethodConnected())
        {
            return 0;
        }

        return reinterpret_cast<UInt64>(m_pInst);   // const + non-const. 같은 메모리.
    }

    [[nodiscard]] bool IsFunctionConnected() const   // check free function + non-capturing lambda
    {
        return IsConnected() && m_pInst == nullptr && m_deleter == nullptr;
    }

    [[nodiscard]] bool IsMethodConnected() const   // check member function
    {
        return IsConnected() && m_pInst && m_deleter == nullptr;
    }

    [[nodiscard]] bool IsFunctorConnected() const
    {
        return IsConnected() && m_pInst && m_deleter;
    }

private:
    union
    {
        std::monostate m_empty = {};
        void*          m_pInst;
        const void*    m_pConstInst;
        SBO            m_storage;
        AnyRef         m_pRef;
    };

    TFunc    m_func    = nullptr;
    TDeleter m_deleter = nullptr;
    TCloner  m_copier  = nullptr;
    UInt64   m_hash    = 0;
};

template<auto TFunc>
[[nodiscard]] decltype(auto) Bind()
{
    using TSign = FunctionTraits<decltype(TFunc)>::Type;

    Delegate<TSign> del = {};
    del.template Connect<TFunc>();
    return del;
}

template<auto TMethod, typename TCaller>
[[nodiscard]] decltype(auto) Bind(TCaller* pInst)
{
    using TSign = FunctionTraits<decltype(TMethod)>::Type;

    Delegate<TSign> del = {};
    del.template Connect<TMethod>(pInst);
    return del;
}

template<auto TMethod, typename TCaller>
[[nodiscard]] decltype(auto) Bind(const TCaller* pInst)
{
    using TSign = FunctionTraits<decltype(TMethod)>::Type;

    Delegate<TSign> del = {};
    del.template Connect<TMethod>(pInst);
    return del;
}

template<typename TCallable>
[[nodiscard]] decltype(auto) Bind(TCallable&& func)
{
    using TDecayed = std::decay_t<TCallable>;
    using TSign    = FunctionTraits<TDecayed>::Type;

    Delegate<TSign> del = {};
    del.Connect(std::forward<TCallable>(func));
    return del;
}

}   // namespace Jam

template<typename TRet, typename... TArg>
struct std::hash<Jam::Delegate<TRet(TArg...)>>
{
    size_t operator()(const Jam::Delegate<TRet(TArg...)>& del) const noexcept
    {
        return std::hash<Jam::UInt64> {}(del.GetHash());
    }
};

template<typename TRet, typename... TArg>
struct ankerl::unordered_dense::hash<Jam::Delegate<TRet(TArg...)>>
{
    size_t operator()(const Jam::Delegate<TRet(TArg...)>& del) const noexcept
    {
        return std::hash<Jam::UInt64> {}(del.GetHash());
    }
};