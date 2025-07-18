#pragma once

namespace jam::detail
{

NODISCARD int EntryPoint(const int argc, char* argv[]);

}

// define this instead of main function in your application
#define JAM_MAIN()                                  \
    int main(const int argc, char* argv[])          \
    {                                               \
        return jam::detail::EntryPoint(argc, argv); \
    }

#define JAM_WIN_MAIN()                                                                              \
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) \
    {                                                                                               \
        char* argv[] = { lpCmdLine };                                                               \
        return jam::detail::EntryPoint(std::size(argv), argv);                                      \
    }