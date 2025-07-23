#pragma once
#include "Buffers.h"

namespace jam
{

class CBufferManager
{
    template <typename Ty>
    static ConstantBuffer GetConstantBuffer()
    {
        static ConstantBuffer buffer;
        if (!buffer.IsValid())
        {
            buffer = ConstantBuffer::Create(sizeof(Ty));
        }
        return buffer;
    }
    
};

}