#include "pch.h"

#include <Windows.h>
#include <iostream>

char k_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int MyStrLen(const char* _str)
{
    int len = 0;
    for (; _str[len] != '\0'; ++len)
        ;
    return len;
}

int main()
{
    std::string      buf;
    constexpr size_t bufSize = 65535;
    buf.reserve(bufSize);
    for (size_t i = 0; i < bufSize - 1; ++i)
    {
        buf.push_back(k_chars[i % (sizeof(k_chars) - 1)]);
    }

    // std::strlen
    ULONGLONG       start = ::GetTickCount64();
    volatile size_t len1  = 0;
    for (int i = 0; i < 10000; ++i)
    {
        len1 = std::char_traits<char>::length(buf.c_str());
    }
    const ULONGLONG end1 = ::GetTickCount64();
    std::cout << "std::strlen: " << (end1 - start) << " ms, len = " << len1 << std::endl;

    // MyStrLen
    start                = ::GetTickCount64();
    volatile size_t len2 = 0;
    for (int i = 0; i < 10000; ++i)
    {
        len2 = MyStrLen(buf.c_str());
    }
    const ULONGLONG end2 = ::GetTickCount64();
    std::cout << "MyStrLen: " << (end2 - start) << " ms, len = " << len2 << std::endl;
}
