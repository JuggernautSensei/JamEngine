#pragma once

#include "Log.h"

int main(char **argv, int argc) 
{
    using namespace jam;
    Log::Initialize();
    Log::Shutdown();
    return 0;
}
