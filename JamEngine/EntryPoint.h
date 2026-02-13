#pragma once

namespace Jam
{

[[nodiscard]] Int32 EntryPoint(Int32 argc, char* argv[]);

}

#define JAM_MAIN()                          \
    int main(const int argc, char* argv[])  \
    {                                       \
        return Jam::EntryPoint(argc, argv); \
    }
