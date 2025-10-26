#pragma once

#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef JAM_DEBUG
#    pragma comment(lib, "../JamEngine/Bin/Debug/JamEngine.lib")
#else
#    pragma comment(lib, "../JamEngine/Bin/Release/JamEngine.lib")
#endif
