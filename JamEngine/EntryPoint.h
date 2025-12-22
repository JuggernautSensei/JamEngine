#pragma once

namespace jam
{

[[nodiscard]] Int32 EntryPoint(Int32 argc, char* argv[]);

}

#define JAM_MAIN()                          \
    int main(const int argc, char* argv[])  \
    {                                       \
        return jam::EntryPoint(argc, argv); \
    }
