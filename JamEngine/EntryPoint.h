#pragma once

namespace jam::details
{

[[nodiscard]] int EntryPoint(int argc, char* argv[]);

}

#define JAM_MAIN()                                  \
    int main(const int argc, char* argv[])          \
    {                                               \
        return jam::details::EntryPoint(argc, argv); \
    }

#define JAM_WIN_MAIN()                                                                              \
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) \
    {                                                                                               \
        char* argv[] = { lpCmdLine };                                                               \
        return jam::details::EntryPoint(std::size(argv), argv);                                      \
    }