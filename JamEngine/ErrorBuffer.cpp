#include "pch.h"

#include "ErrorBuffer.h"

namespace jam
{

struct ErrorBufferContext
{
    std::string buffer;
}* g_pErrBufCtx = nullptr;

}   // namespace jam

namespace jam
{

void ErrorBuffer::Init()
{
    JAM_ASSERT(!g_pErrBufCtx, "ErrorBuffer is already initialized");
    g_pErrBufCtx = JAM_NEW(ErrorBufferContext);
}

void ErrorBuffer::Shutdown()
{
    JAM_ASSERT(g_pErrBufCtx, "ErrorBuffer is not initialized");
    JAM_DELETE(g_pErrBufCtx);
}

void ErrorBuffer::Set(const std::string_view _str)
{
    g_pErrBufCtx->buffer = _str;
}

void ErrorBuffer::Clear()
{
    g_pErrBufCtx->buffer.clear();
}

std::string_view ErrorBuffer::Get()
{
    return g_pErrBufCtx->buffer;
}

}   // namespace jam