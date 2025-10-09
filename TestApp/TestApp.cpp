#include <Windows.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

struct A
{
    inline static int value = 3;
    static A          Create()
    {
        return A();
    }
};

struct B : public A
{
    inline static int value = 5;
    static B          Create()
    {
        return B();
    }
};

struct C : public B
{
    inline static int value = 7;

    static C Create()
    {
        return C();
    }
};

int main()
{
    std::cout << C::Create().value << std::endl;   // 7
    std::cout << B::Create().value << std::endl;   // 5
    std::cout << A::Create().value << std::endl;   // 3
    
    return 0;
}