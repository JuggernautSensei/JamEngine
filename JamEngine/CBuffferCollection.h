#pragma once
#include "Buffers.h"

namespace jam
{

class ConstantBufferCollection
{
public:
    ConstantBufferCollection() = delete;

    template<typename Ty>
    static const ConstantBuffer& Get()
    {
        static ConstantBuffer buffer = []
        {
            ConstantBuffer cbuffer;
            cbuffer.Initialize(sizeof(Ty));
            return cbuffer;
        }();

        return buffer;
    }

    template<typename Ty>
    static void Bind(const eShader _shader, const UInt32 _slot)
    {
        Get<Ty>().Bind(_shader, _slot);
    }
};

}   // namespace jam