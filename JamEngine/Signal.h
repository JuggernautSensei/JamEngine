#pragma once
#include "Delegate.h"

namespace Jam
{

template<typename>
class Signal;

template<typename TRet, typename... TArgs>
class Signal<TRet(TArgs...)>
{
public:
    using DelegateT = Delegate<TRet(TArgs...)>;

    class ScopedConnection
    {
    public:
        ScopedConnection() = default;

        ScopedConnection(
            Signal&          _signal,
            const DelegateT& _delegate)
            : m_pSignal(&_signal)
            , m_delegateHash(_delegate.GetHash())
        {
            m_pSignal->Connect(_delegate);
        }

        ~ScopedConnection()
        {
            if (m_pSignal != nullptr)
            {
                m_pSignal->Disconnect(m_delegateHash);
            }
        }

        ScopedConnection(const ScopedConnection&)            = delete;
        ScopedConnection& operator=(const ScopedConnection&) = delete;

        ScopedConnection(ScopedConnection&& _other) noexcept
            : m_pSignal(_other.m_pSignal)
            , m_delegateHash(_other.m_delegateHash)
        {
            _other.m_pSignal      = nullptr;
            _other.m_delegateHash = 0;
        }

        ScopedConnection& operator=(
            ScopedConnection&& _other) noexcept
        {
            if (this != &_other)
            {
                if (m_pSignal != nullptr)
                {
                    m_pSignal->Disconnect(m_delegateHash);
                }

                m_pSignal             = _other.m_pSignal;
                m_delegateHash        = _other.m_delegateHash;
                _other.m_pSignal      = nullptr;
                _other.m_delegateHash = 0;
            }
            return *this;
        }

    private:
        Signal* m_pSignal      = nullptr;
        UInt64  m_delegateHash = 0;
        friend class Signal;
    };

    void Connect(const DelegateT& _delegate)
    {
        m_delegates[_delegate.GetHash()] = _delegate;
    }

    [[nodiscard]] ScopedConnection ConnectScoped(const DelegateT& _delegate)
    {
        Connect(_delegate);
        return Scoped(*this, _delegate);
    }

    void Disconnect(const DelegateT& _delegate)
    {
        m_delegates.erase(_delegate.GetHash());
    }

    void Disconnect(UInt64 _delegateHash)
    {
        m_delegates.erase(_delegateHash);
    }

    void Emit(TArgs... args) const
    {
        for (const DelegateT& delegate: m_delegates)
        {
            delegate(args...);
        }
    }

    template<typename TFunc>
        requires(!std::is_void_v<TRet> && std::is_invocable_v<TFunc, TRet>)
    void Emit(TFunc&& _func, TArgs... args) const
    {
        for (const DelegateT& delegate: m_delegates)
        {
            _func(delegate(args...));
        }
    }

private:
    CASHashMap<UInt64, DelegateT> m_delegates = {};
};

}   // namespace Jam