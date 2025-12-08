#pragma once

namespace jam::details
{

[[nodiscard]] int EntryPoint(int argc, char* argv[]);

}

/// @brief JAM 엔진의 진입점 매크로.
/// 이 매크로는 main 함수를 정의하며, jam::internal::EntryPoint 함수를 호출합니다.
/// 반드시 프로그램의 진입점에 이 매크로를 사용해야 합니다.

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