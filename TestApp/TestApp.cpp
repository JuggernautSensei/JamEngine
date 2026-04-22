#include <format>
#include <iostream>

int main()
{
    std::string_view helloWorld = "Helloworld!@";
    std::cout << std::format("{}", helloWorld);
}