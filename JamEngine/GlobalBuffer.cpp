#include "pch.h"

#include "GlobalBuffer.h"

namespace jam
{

thread_local FixedString<GlobalBuffer::k_bufferCap> GlobalBuffer::s_errorBuffer;
thread_local FixedString<GlobalBuffer::k_bufferCap> GlobalBuffer::s_tmpBuffer;

}   // namespace jam