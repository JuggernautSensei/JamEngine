#include "pch.h"

#include <cstdio>
#include <stdarg.h>

class GlobalBuffer
{
    using Buffer = char[1024];

public:
    char* TmpFormat(const char* _fmt, ...)
    {
        va_list args;
        va_start(args, _fmt);
        ::vsnprintf(m_buffer[m_curIdx], sizeof(Buffer), _fmt, args);
        va_end(args);
        int returnIdx = m_curIdx;
        m_curIdx      = (m_curIdx + 1) % 5;
        return m_buffer[returnIdx];
    }

private:
    Buffer m_buffer[5] = {};
    int    m_curIdx    = 0;
};

int main()
{
    GlobalBuffer gb;
    printf("%s", gb.TmpFormat("Hello, %s! The answer is %d.", "World", 42));
    printf("%s", gb.TmpFormat("Test number: %d, string: %s", 123, gb.TmpFormat("inner %s", "value")));
    return 0;
}