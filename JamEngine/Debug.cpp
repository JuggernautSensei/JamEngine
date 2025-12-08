#include "pch.h"

#include "Debug.h"

#include "Vendor/backward/backward.hpp"

namespace jam
{

SourceLoc TraceCallerInfo(const Int32 _stackIndex)
{
    JAM_ASSERT(_stackIndex >= 0, "stack index must be non-negative");

    const size_t depth = _stackIndex + 1;

    backward::StackTrace st;
    st.load_here(depth);

    backward::TraceResolver resolver;
    resolver.load_stacktrace(st);

    backward::ResolvedTrace resolved = resolver.resolve(st[_stackIndex]);

    SourceLoc sc;
    sc.filename = resolved.source.filename;
    sc.function = resolved.source.function;
    sc.line     = resolved.source.line;
    sc.col      = resolved.source.col;

    return sc;
}

}   // namespace jam
