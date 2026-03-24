#pragma once
#include <gtest/gtest.h>

#include "../JamEngine/JamEngine.h"
#ifdef JAM_DEBUG
#pragma comment(lib, "../JamEngine/Bin/Debug/JamEngine.lib")
#else
#pragma comment(lib, "../JamEngine/Bin/Release/JamEngine.lib")
#endif
using namespace Jam;

